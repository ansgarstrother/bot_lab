#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <assert.h>

#include "uorc.h"
#include "udp_util.h"
#include "varray.h"

#define MAX_PACKET_SIZE 1600

struct uorc
{
    int sock;

    int estimated_rtt_ms;
    int min_estimated_rtt_ms;
    int max_estimated_rtt_ms;

    varray_t *transactions;
    volatile uint32_t next_transaction_id;

    struct sockaddr_in send_addr;
    pthread_t reader_thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

static int mini(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

static int maxi(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

static int64_t timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

void timespec_addms(struct timespec *ts, long ms)
{
    int sec=ms/1000;
    ms=ms-sec*1000;

    // perform the addition
    ts->tv_nsec+=ms*1000000;

    // adjust the time
    ts->tv_sec+=ts->tv_nsec/1000000000 + sec;
    ts->tv_nsec=ts->tv_nsec%1000000000;
}

void timespec_now(struct timespec *ts)
{
    struct timeval  tv;

    // get the current time
    gettimeofday(&tv, NULL);
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec*1000;
}

static char** strsplit(const char *_s, char c, int *_ntoks)
{
    char *s = strdup(_s);
    int len = strlen(s);

    int ntoks = 0;
    int tokalloc = 4;
    char **toks = (char**) malloc(tokalloc * sizeof(char*));

    toks[ntoks++] = s;

    for (int i = 0; i < len; i++) {
        if (s[i] == c) {
            s[i] = 0;

            if (ntoks >= tokalloc) {
                tokalloc *= 2;
                toks = realloc(toks, tokalloc * sizeof(char*));
            }

            toks[ntoks++] = &s[i+1];
        }
    }

    *_ntoks = ntoks;
    return toks;
}

static void strsplit_free(char **toks)
{
    free(toks[0]);
    free(toks);
}

static int parse_addr(const char *s, struct sockaddr_in *addr, int default_port)
{
    int colon_ntoks;
    char **colon_toks = strsplit(s, ':', &colon_ntoks);

    memset(addr, 0, sizeof(struct sockaddr_in));

    if (colon_ntoks == 2)
        addr->sin_port = htons(atoi(colon_toks[1]));
    else
        addr->sin_port = htons(default_port);

    addr->sin_family = AF_INET;

    if (inet_pton(AF_INET, colon_toks[0], &addr->sin_addr.s_addr) != 1)
        return -1;

    strsplit_free(colon_toks);

    return 0;
}

static uint32_t read_u16(uint8_t *p)
{
    return (p[0]<<8) + (p[1]);
}

static int16_t read_s16(uint8_t *p)
{
    return (p[0]<<8) + (p[1]);
}

static uint32_t read_u32(uint8_t *p)
{
    return (p[0]<<24) + (p[1]<<16) + (p[2]<<8) + (p[3]);
}

static uint64_t read_u64(uint8_t *p)
{
    uint64_t d0 = read_u32(p);
    uint64_t d1 = read_u32(&p[4]);
    return (d0<<32) + d1;
}

static void write_u32(uint8_t *p, uint32_t v)
{
    p[0] = (v>>24)&0xff;
    p[1] = (v>>16)&0xff;
    p[2] = (v>>8)&0xff;
    p[3] = (v)&0xff;
}

static void write_u64(uint8_t *p, uint64_t v)
{
    p[0] = (v>>56)&0xff;
    p[1] = (v>>48)&0xff;
    p[2] = (v>>40)&0xff;
    p[3] = (v>>32)&0xff;
    p[4] = (v>>24)&0xff;
    p[5] = (v>>16)&0xff;
    p[6] = (v>>8)&0xff;
    p[7] = (v)&0xff;
}

static void* reader_thread(void *arg)
{
    uorc_t *uorc = (uorc_t*) arg;
    uint8_t *msg = malloc(MAX_PACKET_SIZE);

    while (1) {
        struct sockaddr_in src_addr;
        socklen_t src_addr_len = sizeof(src_addr);

        ssize_t len = recvfrom(uorc->sock, msg, MAX_PACKET_SIZE, 0, (struct sockaddr*) &src_addr, &src_addr_len);

        if (len < 20)
            continue;

        int pos = 0;
        uint32_t magic = read_u32(&msg[pos]); pos+=4;
        if (magic != 0x0ced0001)
            continue;

        uint32_t transaction_id = read_u32(&msg[pos]); pos+=4;
        uint64_t utime_orc = read_u64(&msg[pos]); pos+=8;
        uint32_t response_id = read_u32(&msg[pos]); pos+=4;

        pthread_mutex_lock(&uorc->mutex);

        for (int idx = 0; idx < varray_size(uorc->transactions); idx++) {
            uorc_response_t *resp = varray_get(uorc->transactions, idx);

            if (resp->transaction_id == transaction_id) {
                // decode transaction
                resp->valid = 1;
                resp->buffer_len = len;
                resp->buffer_pos = pos;
                resp->utime_host = timestamp_now();
                resp->utime_orc = utime_orc;
                resp->response_id = response_id;

                if (resp->buffer != NULL)
                    free(resp->buffer);

                resp->buffer = msg;

                pthread_cond_broadcast(&uorc->cond);

                msg = malloc(MAX_PACKET_SIZE);
            }
        }

        pthread_mutex_unlock(&uorc->mutex);
    }

    return NULL;
}

uorc_t* uorc_create()
{
    uorc_t *uorc = (uorc_t*) calloc(1, sizeof(uorc_t));

    uorc->sock = udp_socket_create();
    uorc->estimated_rtt_ms = 3;
    uorc->max_estimated_rtt_ms = 50;
    uorc->min_estimated_rtt_ms = 2;
    uorc->transactions = varray_create();
    uorc->next_transaction_id = random();

    parse_addr("192.168.237.7:2378", &uorc->send_addr, 2378);

    pthread_mutex_init(&uorc->mutex, NULL);
    pthread_cond_init(&uorc->cond, NULL);
    pthread_create(&uorc->reader_thread, NULL, reader_thread, uorc);

    return uorc;
}

void uorc_destroy(uorc_t *uorc)
{
    close(uorc->sock);
    free(uorc);
}

uorc_response_t *uorc_command(uorc_t *uorc, uint32_t command, const uint8_t *payload, uint32_t payload_len, int timeoutms)
{
    uorc_response_t *resp = (uorc_response_t*) calloc(1, sizeof(uorc_response_t));
    resp->valid = 0;

    pthread_mutex_lock(&uorc->mutex);
    uint32_t transaction_id = uorc->next_transaction_id++;
    resp->transaction_id = transaction_id;
    varray_add(uorc->transactions, resp);
    pthread_mutex_unlock(&uorc->mutex);

    int64_t start_utime = timestamp_now();
    int64_t timeout_utime = start_utime + timeoutms * 1000;

    uint8_t msg[MAX_PACKET_SIZE];
    uint32_t pos = 0;
    write_u32(&msg[pos], 0x0ced0002); pos+=4;
    write_u32(&msg[pos], transaction_id); pos+=4;
    write_u64(&msg[pos], timestamp_now()); pos+=8;
    write_u32(&msg[pos], command); pos+=4;
    memcpy(&msg[pos], payload, payload_len); pos+=payload_len;

    while (1) {
//        printf("transmit %d\n", uorc->estimated_rtt_ms);
        int res = sendto(uorc->sock, msg, pos, 0, (struct sockaddr*) &uorc->send_addr, sizeof(struct sockaddr_in));
        if (res != pos) {
            perror("sendto");
            continue;
        }

        // we're willing to keep waiting until retransmit_timeout.
        struct timespec ts;
        timespec_now(&ts);
        timespec_addms(&ts, uorc->estimated_rtt_ms);

        while (1) {
            pthread_mutex_lock(&uorc->mutex);
            int res = pthread_cond_timedwait(&uorc->cond, &uorc->mutex, &ts);
            pthread_mutex_unlock(&uorc->mutex);

            if (resp->valid || res == ETIMEDOUT) {
                break; // exit wait loop, and retransmit if we still have time.
            }

            // otherwise we had a false alarm (arrival of a response
            // from a different thread, perhaps).. keep waiting.
        }

        if (resp->valid || (timestamp_now() > timeout_utime && timeoutms >= 0))
            break;

        // we must retransmit. Increase rtt estimate.
        uorc->estimated_rtt_ms = mini(uorc->max_estimated_rtt_ms, uorc->estimated_rtt_ms*2);
    }

    pthread_mutex_lock(&uorc->mutex);
    varray_remove(uorc->transactions, resp);
    pthread_mutex_unlock(&uorc->mutex);

    if (resp->valid) {
        int64_t end_utime = timestamp_now();
        uorc->estimated_rtt_ms = maxi(uorc->min_estimated_rtt_ms, (end_utime - start_utime) / 1000 + 1);
        return resp;
    }

    uorc_response_destroy(resp);
    return NULL;
}

void uorc_response_destroy(uorc_response_t *response)
{
    if (response == NULL)
        return;

    if (response->buffer != NULL)
        free(response->buffer);

    free(response);
}

/** You must free the resulting string **/
char *uorc_get_version(uorc_t *uorc)
{
    uorc_response_t *resp = uorc_command(uorc, 0x0002, NULL, 0, -1);

    int sz = resp->buffer_len - resp->buffer_pos;
    char *buf = malloc(sz + 1);
    memcpy(buf, &resp->buffer[resp->buffer_pos], sz);
    buf[sz] = 0;

    uorc_response_destroy(resp);
    return buf;
}


void uorc_get_status(uorc_t *uorc, uorc_status_t *status)
{
    uorc_response_t *resp = uorc_command(uorc, 0x0001, NULL, 0, -1);

    status->utime_orc = resp->utime_orc;
    status->utime_host = resp->utime_host;

    if (resp->buffer_len != 225) {
        printf("short status\n");
        exit(1);
    }

    int pos = resp->buffer_pos;
    uint8_t *buf = resp->buffer;

    status->status_flags = read_u32(&buf[pos]); pos+=4;
    status->debug_chars_waiting = read_u16(&buf[pos]); pos+=2;

    for (int i = 0; i < 13; i++) {
        status->analog_input[i] = read_u16(&buf[pos]); pos+=2;
        status->analog_input_filtered[i] = read_u16(&buf[pos]); pos+=2;
        status->analog_input_filter_alpha[i] = read_u16(&buf[pos]); pos+=2;
    }

    status->simple_digital_values = read_u32(&buf[pos]); pos+=4;
    status->simple_digital_directions = read_u32(&buf[pos]); pos+=4;

    for (int i = 0; i < 3; i++) {
        status->motor_enable[i] = buf[pos]; pos++;
        status->motor_pwm_actual[i] = read_s16(&buf[pos]); pos+=2;
        status->motor_pwm_goal[i] = read_s16(&buf[pos]); pos+=2;
        status->motor_slew_raw[i] = read_u16(&buf[pos]); pos+=2;
    }

    for (int i = 0; i < 2; i++) {
        status->qei_position[i] = read_u32(&buf[pos]); pos+=4;
        status->qei_velocity[i] = read_u32(&buf[pos]); pos+=4;
    }

    for (int i = 0; i < 8; i++) {
        status->fast_digital_mode[i] = buf[pos]; pos++;
        status->fast_digital_config[i] = read_u32(&buf[pos]); pos+=4;
    }

    for (int i = 0; i < 3; i++) {
        status->gyro_integrator[i] = read_u64(&buf[pos]); pos+=8;
        status->gyro_integrator_count[i] = read_u32(&buf[pos]); pos+=4;
    }

    if (pos != resp->buffer_len)
        printf("ACK\n");

    uorc_response_destroy(resp);
}

void uorc_motor_idle(uorc_t *uorc, struct uorc_motor *motor)
{
    uint8_t buf[] = { motor->port,
                      0,
                      0,
                      0 };

    uorc_response_t *resp = uorc_command(uorc, 0x1000, buf, sizeof(buf), -1);

    uorc_response_destroy(resp);
}

void uorc_motor_set_pwm(uorc_motor_t *motor, float v)
{
    assert(v>=-1 && v<=1);

    int pwm = (int) (255*v);
    if (motor->invert)
        pwm *= -1;

    uint8_t buf[] = { motor->port,
                      1,
                      ((pwm>>8)&0xff),
                      pwm & 0xff };

    uorc_response_t *resp = uorc_command(motor->uorc, 0x1000, buf, sizeof(buf), -1);

    uorc_response_destroy(resp);
}

float uorc_motor_get_current(uorc_motor_t *motor, uorc_status_t *status)
{
    float voltage = status->analog_input_filtered[motor->port + 8] / 65535.0 * 3.0;
    float current = voltage * 375.0 / 200.0;

    return current;
}

uint32_t uorc_encoder_get_position(uorc_encoder_t *enc, uorc_status_t *status)
{
    return status->qei_position[enc->port] * (enc->invert ? -1 : 1);
}

float uorc_encoder_get_velocity(uorc_encoder_t *enc, uorc_status_t *status)
{
    // must match firmware.
    const float QEI_VELOCITY_SAMPLE_HZ = 40;

    return status->qei_velocity[enc->port] * (enc->invert ? -1 : 1) * QEI_VELOCITY_SAMPLE_HZ;
}

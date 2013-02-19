#ifndef _COMMON_H
#define _COMMON_H

// Clock rate of the CPU. You can't change this without carefully
// modifying the PLL initialization: this #define is for convenience.
#define CPU_HZ 50000000

// SCHEDULER_HZ must be an integer divisor of both CPU_HZ and 1000000.
#define SCHEDULER_HZ 1000

// This determines the length of the sampling intervals for measuring
// the speed of the quadrature encoders. High values (fast rates) are
// low latency, but also sensitive to quantization noise. Lower values
// (slow rates) have the opposite problem.
#define QEI_VELOCITY_SAMPLE_HZ 40

// PWM frequency is SYSCLK (50mhz) / PWM_PERIOD. 
// Motor driver max rate is 10kHz. PWM_PERIOD must be a multiple of 256.
#define MOTOR_PWM_PERIOD 0x1400

// Picking a good sampling frequency is not trivial. We want the
// average current, but we sample discretely, and the current is
// constantly changing due to PWM. Our strategy is to sample the PWM
// at intervals that allows us to sample the current at different
// phases of the PWM waveform. Let N be the number of different phases
// that we sample at, and let T be the PWM period. We want to sample a
// motor current channel with a period of (K*T + T/N), where K is an
// integer. We have five ADC channels to sample at, so we will sample
// at 5 times this rate. We'll let N be 8. E.g., our sampling period
// is: (K*T + T/N)/5. Letting T = 1/9765 (our nominal PWM rate with a
// MOTOR_PWM_PERIOD = 0x1400), we get the following sampling periods:

// K = 0:  12.8us (way too fast)
// K = 1: 115.2us (still awfully fast, but doable), net sample rate = 8680Hz
// K = 2: 217.6us (comfy), net sample rate = 4595Hz
// ADC is capable of 500khz, i.e., 2us
#define INTADC_PERIOD_US 640
//217

#define ADC_FILTER_SCALE_BITS 14
#define ADC_FILTER_SCALE (1<<ADC_FILTER_SCALE_BITS)

#endif

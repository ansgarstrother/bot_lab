CFLAGS = -std=gnu99 -g -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT -Wall -Wno-unused-parameter -Wno-format-zero-length -pthread -fPIC -O4
LDFLAGS = -lm -lpthread

CC := gcc
LD := gcc

OBJS = uorc.o udp_util.o varray.o

all: orctest


libuorc.a: $(OBJS)
	ar rc $@ $(OBJS)

orctest: main.o $(OBJS)
	$(CC) -o $@ $(OBJS) $< -lpthread

%.o: %.c %.h
	@echo "    [$@]"
	$(CC) $(CFLAGS) -c $<

%.o: %.c
	@echo "    [$@]"
	$(CC) $(CFLAGS) -c $<

clean:
	rm $(OBJS)

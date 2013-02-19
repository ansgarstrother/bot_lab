NKERN_PATH          := ../nkern
NKERN_ARCH_PATH     := $(NKERN_PATH)/archs/cortex_m3
NKERN_PLATFORM_PATH := $(NKERN_PATH)/platforms/lm3s6965

include $(NKERN_PATH)/Makefile.inc

PREFIX    := /opt/arm/bin
CC        := $(PREFIX)/arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -ggdb -g -Os
#-Os
# XXX: -funroll-loops causes problems in init()
# XXX: -fno-common causes program to begin immediately w/o breaking at beginning
# XXX: 2009q1 crashes during compilation
# XXX: try this with 2009q1 -fpromote-loop-indices

CINCLUDE  := -I$(NKERN_PATH)/include -I$(NKERN_ARCH_PATH) -I$(NKERN_PLATFORM_PATH)
CFLAGS    := -I./ -c -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-format-zero-length $(CINCLUDE)
OBJCOPY   := $(PREFIX)/arm-none-eabi-objcopy
OBJDUMP   := $(PREFIX)/arm-none-eabi-objdump

VPATH	+= ../isp

OBJS      := main.o flash.o motor.o fast_digio.o param.o ssi.o mcp23s17.o luminary.o $(NKERN_OBJS) serial.o ethernet.o i2c.o dhcpd.o tlc3548.o command.o get_state.o qei.o digio.o intadc.o extadc.o discover.o heartbeat.o hooks.o shell.o wallfollower.o profile.o autoversion.o shiftbrite.o watchdog.o isp_eth.o

BUILDOBJS := $(OBJS:%=build/%)

all: autoversion.c main.bin


.PHONY:


autoversion.c: .PHONY
	./gitvers.pl FIRMWARE_REVISION . ../nkern > $@

main.out: $(BUILDOBJS) Makefile
	$(CC) -nostartfiles -T link-flash.cmd -o $@ $(BUILDOBJS)

%.bin: %.out
	$(OBJCOPY) $< -O binary $@

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	 rm -rf *.o *~ $(BUILDOBJS) main.bin main.out

# We don't use this target anymore. Instead, use the 'flash' macro
# from within GDB.
#flash:
#	sudo /opt/arm/bin/openocd -f flash.cfg

dump:   main.out
	$(OBJDUMP) -td main.out


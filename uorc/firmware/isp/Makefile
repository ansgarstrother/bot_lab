PREFIX    := /opt/arm/bin
CC        := $(PREFIX)/arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -g -Os

CINCLUDE  := -I$(NKERN_PATH)/include -I$(NKERN_ARCH_PATH) -I$(NKERN_PLATFORM_PATH)
CFLAGS    := -I./ -c -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-format-zero-length $(CINCLUDE)
OBJCOPY   := $(PREFIX)/arm-none-eabi-objcopy
OBJDUMP   := $(PREFIX)/arm-none-eabi-objdump

OBJS      := main_eth.o luminary_isp.o


BUILDOBJS := $(OBJS:%=build/%)

all: main_eth_inc.c


.PHONY:

main_eth.out: $(BUILDOBJS) Makefile
	$(CC) -nostartfiles -T link-ram.cmd -o $@ $(BUILDOBJS)

%.bin: %.out
	$(OBJCOPY) $< -O binary $@

main_eth_inc.c: main_eth.bin
	cat main_eth.bin | ../../tools/bin2c/bin2c __MAIN_ETH > main_eth_inc.c

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	 rm -rf *.o *~ $(BUILDOBJS) main_eth.bin main_eth.out main_eth_inc.c

# We don't use this target anymore. Instead, use the 'flash' macro
# from within GDB.
#flash:
#	sudo /opt/arm/bin/openocd -f flash.cfg

dump:   main.out
	$(OBJDUMP) -td main.out


# NKERN_PATH and NKERN_ARCH_PATH must be defined prior to the inclusion of this file.
# Afterwards, NKERN_OBJS can be used as a dependency list in your Makefile

NKERN_OBJS_NET  := net.o icmp.o ip.o udp.o arp.o tcp.o dhcp.o checksum.o
#
NKERN_OBJS_CORE := nkern.o pprintf.o kprintf.o syscalls.o  wait_list.o mutex.o semaphore.o queue.o sleep_queue.o scheduler.o periodic.o random.o xxtea.o md5.o
NKERN_OBJS_ARCH := arch.o

NKERN_OBJS      := $(NKERN_OBJS_CORE)
NKERN_OBJS      += $(NKERN_OBJS_NET)
NKERN_OBJS      += $(NKERN_OBJS_ARCH)

VPATH           += $(NKERN_PATH)/core $(NKERN_PATH)/net $(NKERN_ARCH_PATH) $(NKERN_PATH)/extra $(NKERN_PLATFORM_PATH)

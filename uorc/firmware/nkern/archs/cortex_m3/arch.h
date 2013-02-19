#ifndef _ARCH_H
#define _ARCH_H

#include <stdint.h>
#include <nkern.h>

typedef struct nkern_task_state nkern_task_state_t;

/** ARM Cortex M3 architecture **/
struct nkern_task_state
{
    uint32_t psp;   // stack pointer
};

typedef uint32_t irqstate_t; // the value of the PRIMASK register

// disable IRQs
static inline void interrupts_disable(irqstate_t *stateptr)
{
	// disable interrupts, preserving the current operating mode
    asm volatile ("mrs ip, BASEPRI   \r\n\t" \
                  "str ip, [%0]      \r\n\t" \
                  "mov ip, #0xc0     \r\n\t" \
                  "msr BASEPRI, ip   \r\n\t" \
                  :
                  :
                  "r"(stateptr)
                  :
                  "memory", "ip");
}

static inline void interrupts_disable2(irqstate_t *stateptr)
{
	// disable interrupts, preserving the current operating mode
    asm volatile ("mrs ip, PRIMASK   \r\n\t" \
                  "str ip, [%0]      \r\n\t" \
                  "cpsid i           \r\n\t"
                  :
                  :
                  "r"(stateptr)
                  :
                  "memory", "ip");
}

// restore IRQ/FIQ state
static inline void interrupts_restore(irqstate_t *stateptr)
{
    asm volatile("msr BASEPRI, %0    \r\n\t"
                 :
                 :
                 "r"(*stateptr)
                 );
}

// restore IRQ/FIQ state
static inline void interrupts_restore2(irqstate_t *stateptr)
{
    asm volatile("msr PRIMASK, %0    \r\n\t"
                 :
                 :
                 "r"(*stateptr));
}

static inline void interrupts_force_enable()
{
    irqstate_t flags = 0;
    interrupts_restore(&flags);
}

static inline void idle()
{
//	asm volatile ("wfi \r\n");
	asm volatile ("nop \r\n");
}

static inline void nop()
{
	asm volatile ("nop \r\n");
}

/** Atomically update the value of bit [0,7] to v [0,1]. basereg should be GPIO_PORTA_DATA_BITS_R, not GPIO_PORTA_DATA_R.**/
static inline void gpio_set_bit(volatile uint32_t *basereg, int bit, int v)
{
    volatile uint32_t *bitreg = (volatile uint32_t*) (((uint32_t) basereg) + (1<<(bit+2)));
    *bitreg = (v << bit);
}

#define IRQ_TASK_SAVE \
    asm volatile ("mrs r12, PSP                   \r\n\t" \
                  "stmdb r12!, {r4-r11}           \r\n\t" \
                  "ldr r0, =nkern_running_task    \r\n\t" \
                  "ldr r0, [r0]                   \r\n\t" \
                  "str r12, [r0]                  \r\n\t");

#define IRQ_TASK_RESTORE \
    asm volatile ("ldr r0, =nkern_running_task    \r\n\t" \
                  "ldr r12, [r0]                  \r\n\t" \
                  "ldr r12, [r12]                 \r\n\t" \
                  "ldmia r12!, {r4-r11}           \r\n\t" \
                  "msr psp, r12                   \r\n\t" \
                  "ldr pc, =0xfffffffd            \r\n\t" \
                  ".ltorg                         \r\n\t");

void nvic_init();
void nvic_set_handler(int handleridx, void (*handler)(void));
void nvic_set_priority(int handleridx, int pri);

#endif


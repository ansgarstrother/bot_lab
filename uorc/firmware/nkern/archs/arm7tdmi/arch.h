#ifndef _ARCH_H
#define _ARCH_H

#include <stdint.h>
#include <nkern.h>

typedef struct nkern_task_state nkern_task_state_t;

/** ARM7TDMI architecture **/
struct nkern_task_state
{
	uint32_t r[15];
	uint32_t cspr, pc;
};

typedef uint32_t irqstate_t;

// disable IRQs
static inline void disable_interrupts(irqstate_t *stateptr)
{
	// disable interrupts, preserving the current operating mode
	asm volatile ("mrs ip, cpsr \r\n"
		      "str ip, [%0] \r\n"
		      "orr ip, ip, #0x80 \r\n"
		      "msr CPSR_c, ip \r\n" 
		      :
		      :
		      "r"(stateptr)
		      :
		      "memory","ip"
                      );
}

// restore IRQ/FIQ state
static inline void enable_interrupts(irqstate_t *stateptr)
{
	asm volatile ("ldr ip, [%0] \r\n"
		      "msr CPSR_c, ip \r\n"
		      :
		      :
		      "r"(stateptr)
		      : "ip"
		);
}

#define enable_all_interrupts enable_interrupts

// disable IRQs *and* FIQs
static inline void disable_all_interrupts(irqstate_t *stateptr)
{
	// disable interrupts, preserving the current operating mode
	asm volatile ("mrs ip, cpsr \r\n"
		      "str ip, [%0] \r\n"
		      "orr ip, ip, #0xc0 \r\n"
		      "msr CPSR_c, ip \r\n" 
		      :
		      :
		      "r"(stateptr)
		      :
		      "memory","ip"
                      );
}

static inline void idle()
{
	asm volatile ("nop \r\n");
}

static inline void nop()
{
	asm volatile ("nop \r\n");
}

/** When writing an IRQ that might want to invoke the scheduler, use a
    __attribute__ naked function with IRQ_TASK_SAVE and _RESTORE as
    the prologue and epilogue.

    These prologues use the stack pointer as intermediate storage, and
    thus is not compatible with stack storage. The general C mechanism
    for using these is thus:

    void pit_irq(void) __attribute__ ((naked));
    void pit_irq(void)
    {
      IRQ_TASK_SAVE;
    
      pit_irq_real();

      IRQ_TASK_RESTORE;
    }

    void pit_irq_real(void) __attribute__ ((noinline));
    void pit_irq_real()
    {
       // now you can use stack storage.
       // you can also call _nkern_scheduler().
    }
**/


/* task save: push the LR on the IRQ stack, then save the task's
   registers using LR as a temp variable. Get the SPSR, pop LR, and
   save those into the task register.
   
   We ought to be able to do "stmia lr!, {r0 - r14}^" and delete the
   "add lr, lr, #60", but this generates a warning.
 */
#define IRQ_TASK_SAVE \
  asm volatile ("stmdb  sp!, {lr}                      \r\n\t" \
		"ldr    lr,  =nkern_running_task       \r\n\t" \
		"ldr    lr,  [lr]                      \r\n\t" \
		"stmia  lr,  {r0 - r14}^               \r\n\t" \
		"add    lr,  lr, #60                   \r\n\t" \
		"mrs    r0,  spsr                      \r\n\t" \
		"ldmia  sp!, {r1}                      \r\n\t" \
		"stmia  lr,  {r0, r1}                  \r\n\t" );

/* task restore: use R0 as a temp register, load the task's SPSR and
   LR, using R1 as a temp for SPSR. Then load the remainder of the
   registers and return to the caller. */
#define IRQ_TASK_RESTORE \
	asm volatile ("ldr     r0,   =nkern_running_task  \r\n\t" \
		      "ldr     r0,   [r0]                 \r\n\t" \
		      "add     r2,   r0, #60              \r\n\t" \
		      "ldmia   r2,   {r1, lr}             \r\n\t" \
		      "msr     spsr_csfx, r1              \r\n\t" \
		      "ldmia   r0,   {r0 - r14}^          \r\n\t" \
		      "nop                                \r\n\t" \
		      "subs    pc,   lr, #4               \r\n\t" \
                      ".ltorg                             \r\n\t");

/* the .ltorg directive helps ensure that the nkern_running_task symbol is defined nearby */

#endif


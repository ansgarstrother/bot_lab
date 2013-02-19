#include <nkern.h>
#include "arch.h"

void _nkern_schedule();

void nkern_initialize_state(nkern_task_t *task, void (*proc)(void*), void *_arg, int stacksize)
{
	nkern_task_state_t *state = &task->state;

	for (int i = 0; i <= 14; i++)
		state->r[i] = (i) | (i<<8) | (i<<16) | (task->pid<<24);

	state->r[13] = ((uint32_t) task->stack) + stacksize - 4;
	state->pc    = (uint32_t) proc+4;
	state->r[0]  = (uint32_t) _arg;
	state->r[10] = (uint32_t) task->stack;
	state->r[14] = (uint32_t) nkern_exit;
	state->cspr  = (uint32_t) 0x1f ; // MODE_SYS, interrupts enabled.
}

void _userspace_reschedule(void) __attribute__ ((naked));
void _userspace_reschedule()
{
	asm volatile("ldr      r0, =nkern_running_task   \r\n\t" \
		     "ldr      r0, [r0]                  \r\n\t" \
		     "stmia    r0, {r0 - r14}            \r\n\t" \
		     "add      r0, r0, #60               \r\n\t" \
		     "add      lr, lr, #4                \r\n\t" \
		     "mrs      r1, cpsr                  \r\n\t" \
		     "stmia    r0, {r1, lr}              \r\n\t" \
		     "msr      cpsr_c, #0x92             \r\n\t");

	_nkern_schedule();

	IRQ_TASK_RESTORE;
}

/** This is used to launch the idle process as the very first process
    in the system; all you need to do is simulate the IRQ_TASK_SAVE
    (which is a NOP, since the state has been filled in by
    nkern_initialize_state), and change to IRQ mode. **/
void _nkern_bootstrap(void) __attribute__ ((naked));
void _nkern_bootstrap()
{
	asm volatile("msr      cpsr_c, #0xD2             \r\n\t");

	_nkern_schedule();

	IRQ_TASK_RESTORE;
}

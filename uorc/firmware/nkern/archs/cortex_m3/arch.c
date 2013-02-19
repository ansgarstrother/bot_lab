#include <string.h>
#include <nkern.h>
#include "arch.h"

#define NHANDLERS 64
__attribute__((aligned(1024)))
uint32_t nvic_vectors[NHANDLERS];

extern void* _vects[];
extern uint32_t _heap_end[], _nkern_stack_top[];
extern void init();

__attribute__ ((section("vectors")))
void *_vects[] = {
    _heap_end,                              // 00    reset stack pointer
    init                                    // 01    Reset program counter
};

// NVIC registers are common to all Cortex-M3 parts.
#define NVIC_INT_TYPE_R         (*((volatile unsigned long *)0xE000E004))
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CAL_R           (*((volatile unsigned long *)0xE000E01C))
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))
#define NVIC_EN1_R              (*((volatile unsigned long *)0xE000E104))
#define NVIC_DIS0_R             (*((volatile unsigned long *)0xE000E180))
#define NVIC_DIS1_R             (*((volatile unsigned long *)0xE000E184))
#define NVIC_PEND0_R            (*((volatile unsigned long *)0xE000E200))
#define NVIC_PEND1_R            (*((volatile unsigned long *)0xE000E204))
#define NVIC_UNPEND0_R          (*((volatile unsigned long *)0xE000E280))
#define NVIC_UNPEND1_R          (*((volatile unsigned long *)0xE000E284))
#define NVIC_ACTIVE0_R          (*((volatile unsigned long *)0xE000E300))
#define NVIC_ACTIVE1_R          (*((volatile unsigned long *)0xE000E304))
#define NVIC_PRI0_R             (*((volatile unsigned long *)0xE000E400))
#define NVIC_PRI1_R             (*((volatile unsigned long *)0xE000E404))
#define NVIC_PRI2_R             (*((volatile unsigned long *)0xE000E408))
#define NVIC_PRI3_R             (*((volatile unsigned long *)0xE000E40C))
#define NVIC_PRI4_R             (*((volatile unsigned long *)0xE000E410))
#define NVIC_PRI5_R             (*((volatile unsigned long *)0xE000E414))
#define NVIC_PRI6_R             (*((volatile unsigned long *)0xE000E418))
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))
#define NVIC_PRI8_R             (*((volatile unsigned long *)0xE000E420))
#define NVIC_PRI9_R             (*((volatile unsigned long *)0xE000E424))
#define NVIC_PRI10_R            (*((volatile unsigned long *)0xE000E428))
#define NVIC_CPUID_R            (*((volatile unsigned long *)0xE000ED00))
#define NVIC_INT_CTRL_R         (*((volatile unsigned long *)0xE000ED04))
#define NVIC_VTABLE_R           (*((volatile unsigned long *)0xE000ED08))
#define NVIC_APINT_R            (*((volatile unsigned long *)0xE000ED0C))
#define NVIC_SYS_CTRL_R         (*((volatile unsigned long *)0xE000ED10))
#define NVIC_CFG_CTRL_R         (*((volatile unsigned long *)0xE000ED14))
#define NVIC_SYS_PRI1_R         (*((volatile unsigned long *)0xE000ED18))
#define NVIC_SYS_PRI2_R         (*((volatile unsigned long *)0xE000ED1C))
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))
#define NVIC_SYS_HND_CTRL_R     (*((volatile unsigned long *)0xE000ED24))
#define NVIC_FAULT_STAT_R       (*((volatile unsigned long *)0xE000ED28))
#define NVIC_HFAULT_STAT_R      (*((volatile unsigned long *)0xE000ED2C))
#define NVIC_DEBUG_STAT_R       (*((volatile unsigned long *)0xE000ED30))
#define NVIC_MM_ADDR_R          (*((volatile unsigned long *)0xE000ED34))
#define NVIC_FAULT_ADDR_R       (*((volatile unsigned long *)0xE000ED38))
#define NVIC_MPU_TYPE_R         (*((volatile unsigned long *)0xE000ED90))
#define NVIC_MPU_CTRL_R         (*((volatile unsigned long *)0xE000ED94))
#define NVIC_MPU_NUMBER_R       (*((volatile unsigned long *)0xE000ED98))
#define NVIC_MPU_R              (*((volatile unsigned long *)0xE000ED9C))
#define NVIC_MPU_ATTR_R         (*((volatile unsigned long *)0xE000EDA0))
#define NVIC_DBG_CTRL_R         (*((volatile unsigned long *)0xE000EDF0))
#define NVIC_DBG_XFER_R         (*((volatile unsigned long *)0xE000EDF4))
#define NVIC_DBG_DATA_R         (*((volatile unsigned long *)0xE000EDF8))
#define NVIC_DBG_INT_R          (*((volatile unsigned long *)0xE000EDFC))
#define NVIC_SW_TRIG_R          (*((volatile unsigned long *)0xE000EF00))

void nkhook_initialize_task_state(nkern_task_t *task, void (*proc)(void*), void *_arg, int stacksize)
{
	nkern_task_state_t *state = &task->state;

    // where will the stack pointer be for this process?
    uint32_t *p = (uint32_t*) (((uint32_t) task->stack) + stacksize - 4*20);

    // push that which IRQ_TASK_RESTORE will pop
    p[0] = 0x0404;      // r4
    p[1] = 0x0505;      // r5
    p[2] = 0x0606;      // r6
    p[3] = 0x0707;      // r7
    p[4] = 0x0808;      // r8
    p[5] = 0x0909;      // r9
    p[6] = 0x0a0a;      // r10
    p[7] = 0x0b0b;      // r11
    // push that which the exception handling hardware will pop
    p[8] = (uint32_t) _arg; // r0
    p[9] = 0x0101;         // r1
    p[10] = 0x0202;         // r2
    p[11] = 0x0303;         // r3
    p[12] = 0x0c0c;         // r12
    p[13] = (uint32_t) nkern_exit;  // LR
    p[14] = (uint32_t) proc;        // PC
    p[15] = 0x01000000;             // xPSR

    uint32_t *q = (uint32_t*) task;
    q[0] = (uint32_t) p;
}

void serial0_putc_spin(char c);

//void _userspace_reschedule(void) __attribute__ ((naked));
void nkhook_userspace_reschedule()
{
    NVIC_INT_CTRL_R |= 1<<26; // pend systick interrupt
    interrupts_force_enable();
}

static void interrupt_unhandled()
{
    uint32_t *psp = 0;

    asm volatile("mrs %0, MSP     \r\n\t"
                 :
                 "=r"(psp)
                 :
                 :
                 "memory", "ip");

//    uint32_t *psp = (uint32_t*) _psp;

    uint32_t r0 = psp[0];
    uint32_t r1 = psp[1];
    uint32_t r2 = psp[2];
    uint32_t r3 = psp[3];
    uint32_t r12 = psp[4];
    uint32_t *exception_lr = (uint32_t*) psp[5];
    uint32_t *exception_pc = (uint32_t*) psp[6];
    uint32_t xpsr = psp[7];

    uint32_t irq_idx = NVIC_INT_CTRL_R & 0xff;

    // irq_idx:
    // 2: NMI
    // 3: Hard Fault (usually a peripheral is accessed that hasn't been powered up)
    // 4:
    // 5: Bus Fault (illegal-memory offset)
    // 15: systick
    // 16+X: external interrupt X

/*
  TODO: Reimplement

    struct iop_t iop;
    iop->write = serial0_write;
    while (1) {
        pprintf(&iop, "NKERN Fatal, unhandled IRQ %3d, LR=%08x, PC=%08x\n",
                (int) irq_idx,
                (unsigned int) exception_lr,
                (unsigned int) exception_pc);
    }
*/

    if (irq_idx == 3) {
        // hard fault
        uint32_t hard_fault_sr = (*((volatile unsigned long *)0xe000ed2c));
        // bit 31 set: debug evt: debugger
        // bit 30 set: bus fault, mem management fault, usage fault
        // bit 1 set: failed vector fetch

        while(1);
    }

    while(1);
}

void nvic_init()
{
    for (int i = 0; i < NHANDLERS; i++)
        nvic_vectors[i] = (uint32_t) interrupt_unhandled;

    memcpy((void*) nvic_vectors, (void*) _vects, sizeof(_vects));

    NVIC_VTABLE_R = (uint32_t) nvic_vectors;
    NVIC_CFG_CTRL_R |= 2; // allow users to pend interrupts.
}

void nvic_set_handler(int handleridx, void (*handler)(void))
{
    nvic_vectors[handleridx] = (uint32_t) handler;
}

void nvic_set_priority(int handleridx, int pri)
{
    // the appropriate register...
    volatile uint32_t *addr = (volatile uint32_t*) (0xe000e400+(handleridx&(~3)));

    // the bit offset.
    int pos = (handleridx&3)*8;

    *addr = ((*addr)&(~(0xff<<pos))) | (pri<<pos);
}

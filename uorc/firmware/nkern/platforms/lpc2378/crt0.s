/* LPC2378 startup code */
	
/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs (program status registers) */
.set  MODE_USR, 0x10            	/* Normal User Mode 					*/
.set  MODE_FIQ, 0x11            	/* FIQ Processing Fast Interrupts Mode 			*/
.set  MODE_IRQ, 0x12            	/* IRQ Processing Standard Interrupts Mode		*/
.set  MODE_SVC, 0x13            	/* Supervisor Processing Software Interrupts Mode 	*/
.set  MODE_ABT, 0x17            	/* Abort Processing memory Faults Mode 			*/
.set  MODE_UND, 0x1B            	/* Undefined Processing Undefined Instructions Mode 		*/
.set  MODE_SYS, 0x1F            	/* System Running Priviledged Operating System Tasks  Mode	*/

.set  I_BIT, 0x80               	/* when I bit is set, IRQ is disabled (program status registers) */
.set  F_BIT, 0x40               	/* when F bit is set, FIQ is disabled (program status registers) */

.arm

.global _vectors

.section .vectors
_vectors:       ldr     PC, reset_addr         
                ldr     PC, und_addr		/* undefined instruction */
                ldr     PC, swi_addr		/* software interrupt */
                ldr     PC, PAbt_Addr		/* prefetch abort */
                ldr     PC, DAbt_Addr		/* data abort */
                nop				/* lpc checksum will be inserted here */
		ldr     PC, [PC, # - 0x0120]	/* irq interrupt, autovectoring via AIC */
                ldr     PC, fiq_addr		/* fast interrupt */

reset_addr:     .word   reset			/* defined in this module below  */
und_addr:       .word   exception_und		/* defined in main.c  */
swi_addr:       .word   exception_swi		/* defined in main.c  */
PAbt_Addr:      .word   exception_pabt		/* defined in main.c  */
DAbt_Addr:      .word   exception_dabt		/* defined in main.c  */
fiq_addr:       .word   exception_fiq		/* defined in main.c  */

                .word   0			/* rounds the vectors and ISR addresses to 64 bytes total  */
_vectors_end:	

exception_und:	b	exception_und
exception_pabt:	b	exception_pabt
exception_dabt:	b	exception_dabt
exception_swi:	b	exception_swi
exception_fiq:	b	on_fiq

.func reset
/* called upon reset, first code to excecute. */
reset:	
		/* Setup a stack for each mode. keep interrupts off for good measure.	*/
	
		msr   CPSR_c, #MODE_UND|I_BIT|F_BIT 	/* Undefined Instruction Mode  */
		ldr   sp, =_stack_und_top

    		msr   CPSR_c, #MODE_ABT|I_BIT|F_BIT 	/* Abort Mode */
		ldr   sp, =_stack_abt_top
	
    		msr   CPSR_c, #MODE_FIQ|I_BIT|F_BIT 	/* FIQ Mode */
		ldr   sp, =_stack_fiq_top
	
    		msr   CPSR_c, #MODE_IRQ|I_BIT   	/* IRQ Mode-- also the NKERN STACK */
		ldr   sp, =_stack_irq_top

    		msr   CPSR_c, #MODE_SVC|I_BIT|F_BIT 	/* Supervisor Mode */
		ldr   sp, =_stack_svc_top

    		msr   CPSR_c, #MODE_SYS|I_BIT     	/* System/User mode - temporarily set to top of ram */
		ldr   sp, =_heap_end
	
gomain:			
		ldr  lr, =exit
		ldr  r0, =main
		bx   r0

		/* If main returns, loop forever */
exit:
		b	.

.endfunc

/********************* stack declaration / allocation ********************/
	.data

.global _nkern_stack
.global _nkern_stack_top		

_stack_irq:	
_nkern_stack:
	.space 2048
_stack_irq_top:	
_nkern_stack_top:
	
_stack_und:
_stack_und_top:

_stack_abt:
_stack_abt_top:	

_stack_fiq:
	.space 128
_stack_fiq_top:	

_stack_svc:
_stack_svc_top:	


	
		
.end


ENTRY(_vectors)

MEMORY 
{
	flash   (rx)   			: ORIGIN = 0x00000000, LENGTH = 512K 

	/* RAM size is 32 bytes less than 32KB, ensuring that we do not 
	   use the RAM reserved for the in-application flash routines */

	ram     (!rx)			: ORIGIN = 0x40000000, LENGTH = 32736
	enetram (!rx)			: ORIGIN = 0x7fe00000, LENGTH = 16K
}

/* define a global symbol _stack_end  */
_stack_end = 0x40008000; 

/* the heap will not be allowed to grow past this point */
_heap_end  = ORIGIN(ram) + LENGTH(ram);

SECTIONS 
{
	.text :
	{
		*(.vectors*)
		*(.rodata)		/* (constants, strings, etc.)  */
		*(.rodata*)		/* (constants, strings, etc.)  */
		*(.text*)		/* (code)  */
		*(.glue_7)		/* (no idea what these are) */
		*(.glue_7t)		/* (no idea what these are) */
		*(.ctors)
		*(.dtors)
		*(.jcr)
		*(.init)
		*(.fini)
		_etext = .;
	} >flash   

	. = ALIGN(4);

	.data :	AT (_etext)  		/* initialized data is stored at _etext (flash) */
	{				/* but is accessed at _data (ram) */
		_data = .;
		*(.data*)
		_edata = .;
	} >ram   

	. = ALIGN(4);

	.bss :				/* zero-ed data */
	{
		__bss_start__ = .;
		*(.bss*)
					
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;

		_end = .;                /* provide this symbol for _sbrk_r implementation*/
	} >ram	


	enet : 
	{ 
		*(.enet) 
	} >enetram
}

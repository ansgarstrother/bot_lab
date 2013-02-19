ENTRY(_vects)

MEMORY
{
        ram     (rwx)                   : ORIGIN = 0x20000000, LENGTH = 64K
}

/* the heap will not be allowed to grow past this point */
_heap_end  = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
	vectors :
	{
                *(.vectors)
                *(.vectors*)
		*(vectors)
	} >ram

        .text :
        {
                *(.text*)
                *(.glue_7)
                *(.glue_7t)
                *(.ctors)
                *(.dtors)
                *(.jcr)
                *(.init)
                *(.fini)
        } >ram


	.rodata :
        {
                *(.rodata .rodata.* .gnu.linkonce.r.*)
        } >ram

	.ARM.exidx :
	{
		*(.ARM.exidx)
	}

	.ARM.extab :
	{
		*(.ARM.extab)
	}

	.stack :
	{
	        . = ALIGN(4);
		_nkern_stack = .;
		. = . + 512;
		. = ALIGN(4);
		_nkern_stack_top = .;
	} >ram

	_etext = .;

        . = ALIGN(4);

        .data : AT (_etext)             /* initialized data is stored at _etext (flash) */
        {                               /* but is accessed at _data (ram) */
		_data = .;
		*(.data)
                *(.data*)
                _edata = .;
        } >ram

        . = ALIGN(4);

        .bss :                          /* zero-ed data */
        {
                __bss_start__ = .;
                *(.bss*)
                *(COMMON)
                . = ALIGN(4);
                __bss_end__ = .;

                _end = .;               /* provide this symbol for _sbrk_r implementation*/
        } >ram

	/* DON'T ADD SECTIONS AFTER _end */
}

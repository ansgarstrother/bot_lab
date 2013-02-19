ENTRY(_vects)

MEMORY 
{
        flash   (rx)                    : ORIGIN = 0x00000000, LENGTH = 256K 
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
	} >flash

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
        } >flash   


	.rodata :
        {
                *(.rodata .rodata.* .gnu.linkonce.r.*)
        } >flash

	.ARM.exidx :
	{
		*(.ARM.exidx)
	}

	.ARM.extab :
	{
		*(.ARM.extab)
	}

	_etext = .;

	.stack :
	{
	        . = ALIGN(4);
		_nkern_stack = .;
		. = . + 512;
		. = ALIGN(4);
		_nkern_stack_top = .;
	}

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

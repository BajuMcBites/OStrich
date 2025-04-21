ENTRY(_start)
SECTIONS
{
	. = 0xFFFF800000000000;
	.text : {
		*(.text)
	}
	.data : {
		*(.data .rodata) QUAD(ADDR(.bss)+SIZEOF(.bss))
	}
	.bss : {
		__bss_start__ = .;
		*(.bss)
		__bss_end__ = .;
	}
}
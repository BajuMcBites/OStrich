SECTIONS
{
    . = 0xffff000000080000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }

    .paging : ALIGN(4096) {
        __paging_start = .;
        *(.paging)  /* Collect all .paging section variables */
        __paging_end = .;
    }

    .stacks : ALIGN(16) {
        __stacks_start = .;
        *(.stacks)
        __stacks_end = .;
    }

    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    . = ALIGN(4096);
    _end = .;

    /* Define heap start after BSS, aligned to 4KB */
    . = ALIGN(4096);
    PROVIDE(__heap_start = .);

    /* Allocate heap size */
    . = . + 100M; /* 2MB heap, adjustable */
    
    PROVIDE(__heap_end = .);
    
    PROVIDE(_frame_table_start = .);
    
    /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}

__bss_size = (__bss_end - __bss_start) >> 3;
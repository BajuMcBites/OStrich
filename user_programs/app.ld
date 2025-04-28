/* app.ld */
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 10M
    stack (rwx) : ORIGIN = 0x0000FFFFFFFF0000, LENGTH = 64K  /* Define the stack region */
}

SECTIONS
{
    /* Code section */
    .text :  
    {
        *(.text._start) /* Force _start first */
        *(.text*)
    } > RAM

    /* Read-only data */
    .rodata : { *(.rodata*) } > RAM

    /* Initialized data */
    .data : {
        *(.data*)
        _cpu_init_hook = .; /* Define the start of this hook area */
    } > RAM

    __bss_start__ = .;
    /* Uninitialized data (zero-initialized) */
    .bss : { *(.bss*) } > RAM
    __bss_end__ = .;

	.heap (NOLOAD) :
	{
		__heap_base = .;
		. = . + 0x50000;
		__heap_limit = .;
	} > RAM
}
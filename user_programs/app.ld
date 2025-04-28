/* app.ld */
ENTRY(_start)

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 50K
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
}
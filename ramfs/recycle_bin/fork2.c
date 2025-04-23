int fork() {
    asm ( "mov	x8, #0x4"); 
    asm (" svc	#0x8 ");
}
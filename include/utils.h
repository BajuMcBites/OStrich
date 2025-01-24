#ifndef	_BOOT_H
#define	_BOOT_H

extern "C" void delay ( unsigned long);
extern "C" void put32 ( volatile unsigned int*, unsigned int );
extern "C" unsigned int get32 (volatile unsigned int*);
extern "C" unsigned long getCoreID();
extern "C" int atomic_exchange(int *ptr, int new_value);
extern "C" void release_lock(int *ptr, int new_value);
extern "C" unsigned long get_el();


extern "C" void irq_init_vectors();
extern "C" void irq_enable();
extern "C" void irq_disable();
extern "C" void monitor(long addr);
extern "C" void outb(int port, int val);

extern int onHypervisor;


#endif  /*_BOOT_H */

#ifndef VM_H
#define VM_H

void patch_page_tables();

#ifdef __cplusplus
extern "C" {
#endif

void create_page_tables();
void init_mmu();

#ifdef __cplusplus
}
#endif

#endif // VM_H

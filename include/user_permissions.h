#ifndef _USER_PERMISSIONS_H
#define _USER_PERMISSIONS_H

#include "stdint.h"

void init_permissions();
void set_user_id(uint32_t id);
bool check_newlib_syscall_perm(uint32_t user, uint32_t syscall_num);
uint32_t get_cur_user();

#endif
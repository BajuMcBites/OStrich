#include "user_permissions.h"
#include "hash.h"
#include "heap.h"

uint32_t cur_user;

static uint64_t uint32_t_hash(uint32_t elem) {
    return elem;
}

static bool uint32_t_equals(uint32_t elem, uint32_t elem2) {
    return elem == elem2;
}

struct Permissions {
    uint64_t newlib_syscall_perms;
} typedef Permissions;

HashMap<uint32_t, Permissions*> *perm_mappings;

void init_permissions() {
    perm_mappings = new HashMap<uint32_t, struct Permissions *>(uint32_t_hash, uint32_t_equals);
    struct Permissions *root = (struct Permissions*) kmalloc(sizeof(struct Permissions));
    root->newlib_syscall_perms = ~(0);
    perm_mappings->put(0, root);

    struct Permissions *user = (struct Permissions*) kmalloc(sizeof(struct Permissions));
    user->newlib_syscall_perms = 0;
    perm_mappings->put(404, user);
}

void set_user_id(uint32_t id) {
    cur_user = id;
}

uint32_t get_cur_user() {
    return cur_user;
}

bool check_newlib_syscall_perm(uint32_t user, uint32_t syscall_num) {
    return perm_mappings->get(user)->newlib_syscall_perms & (1 << syscall_num);
}

#include "fs.h"

#include "event.h"
#include "libk.h"
#include "ramfs.h"
#include "utils.h"

static inodeListNode* inode_list;
Lock* inode_list_lock = nullptr;

volatile uint64_t inode_numbers = 0;
SpinLock inode_number_lock;

/**
 * opens a file specified by file name and returns a file* to it
 * 
 * This as of right now does not verify the file exists, only checks if 
 *
 * 
 */
void kfopen(char* file_name, Function<void(file*)> w) {

    if (inode_list_lock == nullptr) { /* hacky way to use one lock but only on first go */
        inode_number_lock.lock();
        if (inode_list_lock == nullptr) {
            inode_list_lock = new Lock;
        }
        inode_number_lock.unlock();
    }

    /* TODO: verify file exists and get characteristics */

    inode_list_lock->lock([=]() {

        inodeListNode* n = inode_list;
        inodeListNode* prev = nullptr;

        while (n != nullptr && K::strncmp(n->file_name, file_name, PATH_MAX) != 0) {

            if (n->inode->refs == 0) { /* clean up 0 ref inodes */
                delete n->file_name;
                if (prev == nullptr) {
                    inode_list = n->next;
                    delete n;
                    n = inode_list;
                } else {
                    prev->next = n->next;
                    delete n;
                    n = prev->next;
                }
            }

            prev = n;
            n = n->next;
        }

        file* f = new file;

        if (n != nullptr) {
            f->inode = n->inode;
            n->inode->lock.lock();
            n->inode->refs++;
            n->inode->lock.unlock();
            inode_list_lock->unlock();
            create_event(w, f);
            return;
        }

        inode* node = new inode;

        inode_number_lock.lock();
        node->inode_number = inode_numbers++;
        inode_number_lock.unlock();
        node->refs = 1;

        inodeListNode* list_node = new inodeListNode;

        int path_length = K::strnlen(file_name, PATH_MAX - 1);
        char* file_name_cpy = (char*)kmalloc(path_length + 1);
        K::strncpy(file_name_cpy, file_name, PATH_MAX);

        list_node->file_name = file_name_cpy;
        list_node->inode = node;

        if (inode_list == nullptr) {
            inode_list = list_node;
        } else {
            prev->next = list_node;
        }

        inode_list_lock->unlock();
        printf("file opened on core %d\n", getCoreID());
        create_event(w, f);
    });
}

void kfclose(file* file) {
    file->inode->lock.lock();
    file->inode->refs--;
    file->inode->lock.unlock();
    delete file;
}

void get_file_name(file* file, Function<void(char*)> w) {
    if (inode_list_lock == nullptr) { /* hacky way to use one lock but only on first go */
        inode_number_lock.lock();
        if (inode_list_lock == nullptr) {
            inode_list_lock = new Lock;
        }
        inode_number_lock.unlock();
    }

    inode_list_lock->lock([=]() {

        inodeListNode* n = inode_list;
        while (n != nullptr && n->inode->inode_number != file->inode->inode_number) {
            n = n->next;
        }

        K::assert(n != nullptr, "called get file name on a non opened file");

        create_event(w, n->file_name);
        inode_list_lock->unlock();
    });
}


/**
 * kread should read a file into the specified buffer by routing a read call to the
 * correct filesystem handler, ie userspace main fs, device read
 */
void read(file* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    get_file_name(file, [=](char* file_name) {
        int path_length = K::strnlen(file_name, PATH_MAX - 1);
        char* file_name_cpy = (char*)kmalloc(path_length + 1);
        K::strncpy(file_name_cpy, file_name, PATH_MAX);
    
        file_name = file_name_cpy;
    
        char* next;
    
        auto work = [=](int ret) {
            kfree(file_name_cpy);
            create_event<int>(w, ret);
        };
    
        if (file_name == nullptr) {
            create_event<int>(work, INVALID_FILE);
            return;
        }
    
        if (*file_name == '/') { /* starting from root */
            file_name++;
            next = K::strntok(file_name, '/', ENTRY_MAX + 1);
    
            if (next == nullptr) {
                create_event<int>(work, INVALID_FILE);
                return;
            }
    
            if (K::strncmp(file_name, "dev", ENTRY_MAX) == 0) {
                read_dev(next, offset, buf, n, work);
                return;
            } else {
                create_event<int>(work, NOT_IMPLEMENTED_FS);
                return;
            }
        } else { /* starting from somewhere else */
            /* TODO */
            K::assert(false, "non root fs not implemented");
        }
    });
}

/**
 * should take in the name of the file without /dev/, so "/dev/ramfs/temp.txt" would be
 * passed in as "ramfs/temp.txt"
 */
void read_dev(char* file_name, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    int err;
    char* next = K::strntok(file_name, '/', ENTRY_MAX);

    if (K::strncmp(file_name, "ramfs", ENTRY_MAX) == 0) { /* ramfs file */
        int ramfs_index = get_ramfs_index(next);
        if (ramfs_index == -1) {
            create_event<int>(w, INVALID_FILE);
            return;
        }

        uint64_t file_size = ramfs_size(ramfs_index);
        if (offset >= file_size) {
            create_event<int>(w, INVALID_OFFSET);
            return;
        }

        int64_t read_n = K::min(n, file_size - offset);

        err = ramfs_read(buf, offset, read_n, ramfs_index);
        if (err != 0) {
            create_event<int>(w, OTHER_FAIL);
            return;
        }

        create_event<int>(w, SUCCESSFUL_READ);
        return; /* done reading from ramfs */

    } else {
        create_event<int>(w, NOT_IMPLEMENTED_FS);
        return;
    }
}
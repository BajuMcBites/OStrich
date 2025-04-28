#ifndef _BITMAP_H
#define _BITMAP_H

#include "heap.h"

class Bitmap {
   public:
    Bitmap(int size) {
        this->size = size;
        container = (char *)kcalloc(1, (size + 7) / 8);
    }

    ~Bitmap() {
        kfree(container);
    }

    long scan_and_flip() {
        char *cur = container;
        int i = 0;
        while (i < size) {
            if (i % 8 == 0) {
                cur = container + i / 8;
            }

            char mask = (0x1 << (i % 8));
            if (!(*cur & mask)) {
                *cur |= mask;
                return i;
            }

            i++;
        }

        return -1;
    }

    void free(int bit_number) {
        int container_index = bit_number / 8;
        int bit_offset = bit_number % 8;
        char mask = ~(0x1 << bit_offset);

        container[container_index] &= mask;
    }

   private:
    char *container;
    uint64_t size;
};

#endif /*_BITMAP_H */

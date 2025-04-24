#include "swap.h"

#include "event.h"
#include "libk.h"
#include "printf.h"
#include "sdio.h"

/**
 * writes the page cooresponding to swap id into the kvaddr
 */
void Swap::write_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w) {
    lock->lock([=]() {
        long sector_index = bitmap->scan_and_flip();
        if (sector_index == -1) {
            K::assert(false, "Swap is full");
        }

        sector_index = (uint32_t)sector_index;
        uint32_t sector = sector_index_to_sector(sector_index);
        map->put(swap_id, sector);
        int ret = sd_write_block((unsigned char*)kvaddr, sector, 8);
        K::assert(ret == PAGE_SIZE, "writing to swap partition failed");
        create_event(w, 0);
        lock->unlock();
    });
}

/**
 * reads the page cooresponding to swap id into the kvaddr
 */
void Swap::read_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w) {
    lock->lock([=]() {
        long sector = swap_id_to_sector(swap_id);
        if (sector == -1) {
            K::memset(kvaddr, 0, PAGE_SIZE);
        } else {
            sector = (uint32_t)sector;
            int ret = sd_read_block(sector, (unsigned char*)kvaddr, 8);
            if (ret != PAGE_SIZE) {
                K::assert(false, "");
            }
            bitmap->free(sector_to_sector_index(sector));
            map->remove(swap_id);
        }
        create_event(w, 1);
        lock->unlock();
    });
}

/**
 * removes and frees swap_id if it is currently stored in swap
 */
void Swap::clear_swap(uint64_t swap_id, Function<void(void)> w) {
    lock->lock([=]() {
        uint32_t sector = (uint32_t)swap_id_to_sector(swap_id);
        bitmap->free(sector_to_sector_index(sector));
        map->remove(swap_id);
        create_event(w);
        lock->unlock();
    });
}

/**
 * returns a unique swap id that can be written or read from by vm
 */
void Swap::get_swap_id(Function<void(uint64_t)> w) {
    lock->lock([=]() {
        uint64_t swap_id = unused_ids++;
        create_event(w, swap_id);
        lock->unlock();
    });
}

/**
 * swap id, to the physical sector where the page is stored
 */
long Swap::swap_id_to_sector(uint64_t swap_id) {
    return map->get(swap_id);
}

inline uint32_t Swap::sector_to_sector_index(uint32_t sector) {
    return (sector - starting_sector) / 8;
}

inline uint32_t Swap::sector_index_to_sector(uint32_t sector_index) {
    return sector_index * 8 + starting_sector;
}

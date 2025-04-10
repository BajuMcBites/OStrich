#include "swap.h"
#include "event.h"
#include "sdio.h"
#include "printf.h"

void Swap::write_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w) {
    lock->lock([=]() {
        int sector_index = bitmap->scan_and_flip();
        if (sector_index == -1) {
            K::assert(false, "Swap is full");
        }

        uint32_t sector = sector_index_to_sector(sector_index);

        map->put(swap_id, sector);
        int err = sd_write_block((unsigned char*) kvaddr, sector, 4);
        K::assert(err != -1, "writing to swap partition failed");
        create_event(w, 0);
        lock->unlock();
    });

}

void Swap::read_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w) {
    lock->lock([=]() {
        uint32_t sector = swap_id_to_sector(swap_id);
        sd_read_block(sector, (unsigned char*) kvaddr, 4);

        bitmap->free(sector_to_sector_index(sector));
        map->remove(swap_id);

        create_event(w, 1);
        lock->unlock();
    });
}

void Swap::clear_swap(uint64_t swap_id, Function<void(void)> w) {
    lock->lock([=]() {
        uint32_t sector = swap_id_to_sector(swap_id);
        bitmap->free(sector_to_sector_index(sector));
        map->remove(swap_id);
        create_event(w);
        lock->unlock();
    });
}

void Swap::get_swap_id(Function<void(uint64_t)> w) {
    lock->lock([=]() {
        uint64_t swap_id = unused_ids++;
        create_event(w, swap_id);
        lock->unlock();
    });
}

uint64_t Swap::swap_id_to_sector(uint64_t swap_id) {
    return (uint64_t) map->get(swap_id);
}

inline uint32_t sector_to_sector_index(uint32_t sector) {
    return sector / 4;
}

inline uint32_t sector_index_to_sector(uint32_t sector_index) {
    return sector_index * 4;
}



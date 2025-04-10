#ifndef _SWAP_H
#define _SWAP_H

#include "stdint.h"
#include "partition.h"
#include "bitmap.h"
#include "function.h"
#include "atomic.h"
#include "hash.h"

#define SWAP_ID 0xb8

class Swap {
    public:
     Swap(uint64_t num_sectors) {
        this->num_sectors = num_sectors;
        create_partition(num_sectors, SWAP_ID);
        PartitionEntry partitions[4];
        int ret = read_partition_table(partitions);
        starting_sector = 0;
        for (int i = 0; i < 4; i++) {
            if (partitions[i].system_id == SWAP_ID && partitions[i].total_sectors == num_sectors) {
                starting_sector = partitions[i].relative_sector;
            }
        }

        this->bitmap = new Bitmap(num_sectors / 4);
        this->lock = new Lock;
        this->map = new HashMap<uint64_t, uint32_t>(uint64_t_hash, uint64_t_equals, 1000);
        unused_ids = 1;
     }

    void write_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w);
    void read_swap(uint64_t swap_id, void* kvaddr, Function<void(void)> w);
    void clear_swap(uint64_t swap_id, Function<void(void)> w);
    void get_swap_id(Function<void(uint64_t)> w);

    uint64_t swap_id_to_sector(uint64_t swap_id); 

    private:
     uint64_t num_sectors;
     uint64_t starting_sector;
     Bitmap* bitmap;
     Lock* lock;
     HashMap<uint64_t, uint32_t>* map;
     uint32_t unused_ids;

    ~Swap() {
        delete bitmap;
        delete lock;
    }
     
     
};

inline uint32_t sector_to_sector_index(uint32_t sector);

inline uint32_t sector_index_to_sector(uint32_t sector_index);

#endif /* _SWAP_H */

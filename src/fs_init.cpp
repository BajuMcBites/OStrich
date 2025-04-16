#include "fs_init.h"

#include "../filesystem/filesys/FileSystem.h"
#include "../filesystem/interface/BlockManager.h"
#include "libk.h"
#include "partition.h"
#include "printf.h"
#include "sdio.h"

/**
 * @brief Get the filesystem partition information.
 *
 * @param fs_start_sector Pointer to store the start sector of the filesystem partition.
 * @param fs_num_blocks Pointer to store the number of blocks in the filesystem partition.
 */
void get_fs_partition_info(int& fs_start_sector, int& fs_num_blocks) {
    // Read partition table.
    PartitionEntry partitions[4];
    int ret = read_partition_table(partitions);
    if (ret < 0) {
        printf("Failed to read partition table.\n");
        return;
    }

    // Find the filesystem partition.
    for (int i = 0; i < 4; i++) {
        if (partitions[i].system_id == FS_PARTITION_ID) {
            fs_start_sector = partitions[i].relative_sector;
            fs_num_blocks = partitions[i].total_sectors;
        }
    }

    K::assert(fs_start_sector != -1 && fs_num_blocks != -1, "Failed to find filesystem partition.");
}

void fs_init() {
    int fs_start_sector = -1;
    int fs_num_blocks = -1;
    get_fs_partition_info(fs_start_sector, fs_num_blocks);
    auto blockManager = new fs::BlockManager(fs_num_blocks, fs_start_sector);
    fs::FileSystem::getInstance(blockManager);
}
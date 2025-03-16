#include "partition.h"

#include "libk.h"
#include "printf.h"
#include "sdio.h"
int validate_mbr_signature(uint8_t* signature) {
    // Reverse order of bytes due to little endianness.
    return (signature[1] << 8 | signature[0]) == MBR_SIGNATURE;
}

MasterBootRecord* read_mbr(unsigned char* mbr) {
    int ret;

    ret = sd_read_block(MBR_SECTOR, mbr, 1);
    if (ret < 0) {
        return nullptr;
    }

    // Print out mbr as hex with nice formatting
    // for (int i = 0; i < 512; i++) {
    //     if (i % 16 == 0) {
    //         printf("\n");
    //     }
    //     printf("%02x ", mbr[i]);
    // }
    // printf("\n");

    return reinterpret_cast<MasterBootRecord*>(mbr + OFFSET_MBR);
}

int read_partition_table(PartitionEntry partitions[4]) {
    unsigned char mbr_block[512];
    MasterBootRecord* mbr = read_mbr(mbr_block);
    if (!mbr) {
        return SD_ERROR;
    }

    // Validate MBR signature
    if (!validate_mbr_signature(mbr->signature)) {
        return SD_ERROR;
    }

    // Copy partition entries
    K::memcpy(partitions, mbr->partitions, sizeof(PartitionEntry) * NUM_PARTITIONS);

    // Count valid partitions (non-zero entries)
    int count = 0;
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        if (mbr->partitions[i].system_id != 0) {
            count++;
        }
    }

    return count;
}

int create_partition(uint32_t num_sectors, uint8_t system_id) {
    unsigned char mbr_block[512];
    MasterBootRecord* mbr = read_mbr(mbr_block);
    if (!mbr) {
        printf("create_partition(): Failed to read MBR\n");
        return SD_ERROR;
    }

    // Validate MBR signature
    if (!validate_mbr_signature(mbr->signature)) {
        printf("create_partition(): Invalid MBR signature\n");
        return SD_ERROR;
    }

    // Find first empty partition slot
    int slot = 0;
    int start_sector = 1;
    for (slot = 0; slot < NUM_PARTITIONS; slot++) {
        bool is_free = mbr->partitions[slot].system_id == 0;
        if (is_free) {
            break;
        }
        start_sector = mbr->partitions[slot].relative_sector + mbr->partitions[slot].total_sectors;
    }

    if (slot == NUM_PARTITIONS) {
        printf("create_partition(): No free partition slots\n");
        return SD_ERROR;
    }

    if (start_sector + num_sectors > 1024 * 1024) {
        printf("create_partition(): Not enough space on SD card\n");
        return SD_ERROR;
    }

    // Create new partition entry
    mbr->partitions[slot].boot_indicator = 0x00;
    mbr->partitions[slot].starting_head = 0;
    mbr->partitions[slot].starting_sector = 0;
    mbr->partitions[slot].starting_cylinder = 0;
    mbr->partitions[slot].system_id = system_id;
    mbr->partitions[slot].ending_head = 0xFF;
    mbr->partitions[slot].ending_sector = 0xFFFF;
    mbr->partitions[slot].ending_cylinder = 0xFFFF;
    mbr->partitions[slot].relative_sector = start_sector;
    mbr->partitions[slot].total_sectors = num_sectors;

    // Write updated MBR back to disk
    if (sd_write_block(mbr_block, MBR_SECTOR, 1) < 0) {
        printf("create_partition(): Failed to write MBR to SD card\n");
        return SD_ERROR;
    }

    printf("create_partition(): Created partition at sector %d with %d sectors\n", start_sector,
           num_sectors);

    return SD_OK;
}

int get_num_partition_blocks(int system_id) {
    unsigned char mbr_block[512];
    MasterBootRecord* mbr = read_mbr(mbr_block);
    if (!mbr) {
        return SD_ERROR;
    }

    for (int i = 0; i < NUM_PARTITIONS; i++) {
        if (mbr->partitions[i].system_id == system_id) {
            return mbr->partitions[i].total_sectors;
        }
    }

    return SD_ERROR;
}

int delete_partition(int partition_number) {
    if (partition_number < 0 || partition_number >= NUM_PARTITIONS) {
        return SD_ERROR;
    }

    unsigned char mbr_block[512];
    MasterBootRecord* mbr = read_mbr(mbr_block);
    if (!mbr) {
        return SD_ERROR;
    }

    // Validate MBR signature
    if (!validate_mbr_signature(mbr->signature)) {
        return SD_ERROR;
    }

    // Zero out the partition entry
    K::memset(&mbr->partitions[partition_number], 0, sizeof(PartitionEntry));

    // Write updated MBR back to disk
    if (sd_write_block(mbr_block, MBR_SECTOR, 1) < 0) {
        return SD_ERROR;
    }

    return SD_OK;
}
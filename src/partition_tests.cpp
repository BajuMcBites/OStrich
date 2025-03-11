#include "partition.h"
#include "printf.h"
#include "sdio.h"

#define TEST_PARTITION_START_SECTOR 2
#define TEST_PARTITION_NUM_SECTORS 16
#define TEST_PARTITION_SYSTEM_ID 0x83

// Test reading the MBR and validating partition entries
void test_read_mbr() {
    unsigned char mbr_block[512];
    MasterBootRecord* mbr = read_mbr(mbr_block);
    if (!mbr) {
        printf("test_read_mbr(): Failed to read MBR\n");
        return;
    }

    // Verify MBR signature
    if (!validate_mbr_signature(mbr->signature)) {
        printf("test_read_mbr(): Invalid MBR signature: %x (expected %x)\n",
               (mbr->signature[0] | (mbr->signature[1] << 8)), MBR_SIGNATURE);
        return;
    }

    // Print partition table entries
    printf("Partition Table:\n");
    for (int i = 0; i < 4; i++) {
        // JANK: using ptrs avoids implicit memcpy bc we don't support it rn 💀💀
        PartitionEntry* entry = &mbr->partitions[i];
        if (entry->system_id != 0) {  // Non-empty partition
            printf("Partition %d:\n", i + 1);
            printf("  System ID: 0x%02x\n", entry->system_id);
            printf("  Boot Indicator: 0x%02x\n", entry->boot_indicator);
            printf("  Starting Sector: %u\n", entry->relative_sector);
            printf("  Total Sectors: %u\n", entry->total_sectors);
        }
    }

    printf("test_read_mbr(): passed\n");
}

// Test creating a new partition
void test_create_partition() {
    printf("Testing partition creation...\n");

    // Create a small test partition (1MB = 2048 sectors)
    uint32_t num_sectors = TEST_PARTITION_NUM_SECTORS;
    uint8_t system_id = TEST_PARTITION_SYSTEM_ID;  // Linux partition type

    int ret = create_partition(num_sectors, system_id);
    if (ret != SD_OK) {
        printf("test_create_partition(): Failed to create partition, error %d\n", ret);
        return;
    }

    // Verify the partition was created by reading the partition table
    PartitionEntry partitions[4];
    ret = read_partition_table(partitions);
    if (ret < 0) {
        printf("test_create_partition(): Failed to read partition table, error %d\n", ret);
        return;
    }

    // Find our newly created partition
    bool found = false;
    for (int i = 0; i < 4; i++) {
        if (partitions[i].system_id == system_id && partitions[i].total_sectors == num_sectors) {
            found = true;
            break;
        }
    }

    if (!found) {
        printf("test_create_partition(): Created partition not found in table\n");
        return;
    }

    printf("test_create_partition(): passed\n");
}

// Test deleting a partition
void test_delete_partition() {
    printf("Testing partition deletion...\n");

    // First, read the current partition table
    PartitionEntry partitions[4];
    int ret = read_partition_table(partitions);
    if (ret < 0) {
        printf("test_delete_partition(): Failed to read partition table, error %d\n", ret);
        return;
    }

    // Find a non-essential partition to delete (for testing)
    int partition_to_delete = -1;
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        // Look for our test partition from the creation test
        if (partitions[i].system_id == TEST_PARTITION_SYSTEM_ID &&
            partitions[i].total_sectors == TEST_PARTITION_NUM_SECTORS) {
            partition_to_delete = i;
            break;
        }
    }

    if (partition_to_delete == -1) {
        printf("test_delete_partition(): No suitable partition found for deletion test\n");
        return;
    }

    // Delete the partition
    ret = delete_partition(partition_to_delete);
    if (ret != SD_OK) {
        printf("test_delete_partition(): Failed to delete partition, error %d\n", ret);
        return;
    }

    // Verify the partition was deleted
    ret = read_partition_table(partitions);
    if (ret < 0) {
        printf("test_delete_partition(): Failed to read partition table after deletion, error %d\n",
               ret);
        return;
    }

    if (partitions[partition_to_delete].system_id != 0) {
        printf("test_delete_partition(): Partition still exists after deletion\n");
        return;
    }

    printf("test_delete_partition(): passed\n");
}

// Run all partition-related tests
void partitionTests() {
    printf("Starting partition tests...\n");
    test_read_mbr();
    test_create_partition();
    test_delete_partition();
    printf("Partition tests complete\n");
}
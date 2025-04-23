#include "stdint.h"

#define FS_PARTITION_ID 0x0C
#define SWAP_PARTITION_ID 0x0D

// Partition table entry structure
typedef struct {
    uint8_t boot_indicator;  // Boot flag: 0x80 active, 0x00 inactive
    uint8_t starting_head;
    uint8_t
        starting_sector;  // Lower 6 bits are sector number, upper 2 bits are high bits of cylinder
    uint8_t starting_cylinder;  // Lower 8 bits of cylinder
    uint8_t system_id;          // Partition type
    uint8_t ending_head;
    uint8_t
        ending_sector;  // Lower 6 bits are sector number, upper 2 bits are high bits of cylinder
    uint8_t ending_cylinder;   // Lower 8 bits of cylinder
    uint32_t relative_sector;  // Starting LBA of the partition
    uint32_t total_sectors;    // Size of the partition in sectors
} __attribute__((packed)) PartitionEntry;

// DO NOT REORDER THESE FIELDS.
// Get a pointer via read_mbr().
typedef struct {
    PartitionEntry partitions[4];
    uint8_t signature[2];
} __attribute__((packed)) MasterBootRecord;

// DO NOT CHANGE THIS.
#define OFFSET_MBR 0x1BE

// Partition management functions

/**
 * @brief Read the Master Boot Record (MBR) from the SD card.
 *
 * This function reads the first sector of the SD card, which contains the MBR,
 * and validates its signature. The MBR contains the partition table and other
 * information about the disk layout.
 *
 * @param mbr Pointer to a unsigned char array to store the read MBR data.
 *
 * @return On success, returns a pointer to the MBR.
 *         On failure, returns nullptr.
 *
 * @note The MBR is stored in the first sector of the SD card.
 * @note The returned pointer points to start of the input array.
 */
MasterBootRecord* read_mbr(unsigned char* mbr);

/**
 * @brief Validate the MBR signature.
 *
 * This function validates the signature of the Master Boot Record (MBR).
 * The signature is a 16-bit value stored in the last two bytes of the MBR.
 *
 * @param signature Pointer to the signature array.
 *
 * @return 1 if the signature is valid, 0 otherwise.
 */
int validate_mbr_signature(uint8_t* signature);

/**
 * @brief Read the partition table from the SD card.
 *
 * This function reads the partition table from the MBR and stores the partition
 * entries in the provided array. It validates the MBR signature before copying
 * the partition entries.
 *
 * @param partitions Array to store the found partition entries.
 *
 * @return Number of partitions found, or negative error code.
 */
int read_partition_table(PartitionEntry partitions[4]);

/**
 * @brief Create a new partition on the SD card.
 *
 * This function creates a new partition entry in the MBR with the specified
 * starting sector, number of sectors, and system ID. It finds the first empty
 * partition slot and writes the new partition entry to the MBR.
 *
 * @param start_sector Starting sector for the new partition.
 * @param num_sectors Number of sectors for the partition.
 * @param system_id Partition type identifier.
 *
 * @return SD_OK on success, error code on failure.
 */
int create_partition(uint32_t num_sectors, uint8_t system_id);

/**
 * @brief Delete a partition from the SD card.
 *
 * This function deletes a partition entry from the MBR by zeroing out the
 * specified partition slot. It validates the MBR signature before modifying
 * the partition entry.
 *
 * @param partition_number Partition number to delete (0-3).
 *
 * @return SD_OK on success, error code on failure.
 */
int delete_partition(int partition_number);

/**
 * @brief Get the number of blocks in a partition.
 *
 * This function returns the number of blocks in a partition based on the system ID.
 *
 * @param system_id System ID of the partition.
 *
 * @return Number of blocks in the partition, or negative error code.
 */
int get_num_partition_blocks(int system_id);

/**
 * @brief Get the start sector of a partition.
 *
 * This function returns the start sector of a partition based on the system ID.
 *
 * @param system_id System ID of the partition.
 *
 * @return Start sector of the partition, or negative error code.
 */
int get_partition_start_sector(int system_id);

/**
 * @brief Set the partition table on the SD card.
 *
 * This function sets the partition table on the SD card with the specified filesystem and swap
 * sizes. It creates two partitions: one for the filesystem and one for the swap.
 *
 * @param fs_bytes Size of the filesystem in bytes.
 * @param swap_bytes Size of the swap in bytes.
 *
 * @return SD_OK on success, error code on failure.
 */
int set_partition_table(uint64_t fs_bytes, uint64_t swap_bytes);

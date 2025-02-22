#ifndef _SDIO_H
#define _SDIO_H

#include "peripherals/sdio.h"

// SW return codes (unrelated to HW).
#define SD_OK 0
#define SD_TIMEOUT -1
#define SD_ERROR -2

/**
 * @brief Reads one or more blocks from the SD card into a provided buffer.
 *
 * This function performs a block-based read from the SD card starting at the specified block address.
 * It handles both high-capacity (block-addressed) and standard-capacity (byte-addressed) cards by checking
 * the card's capabilities and adjusting the command sequence accordingly.
 *
 * @param block_addr The starting block address to read from. 
 * @param buffer     Pointer to the destination buffer where the read data will be stored. This buffer must be large enough
 *                   to hold (num_blocks * SD_BLK_SIZE) bytes.
 * @param num_blocks The number of blocks to read from the SD card.
 *
 * @return On success, returns the total number of bytes read (num_blocks * SD_BLK_SIZE).
 *         On failure, returns a negative error code (e.g., SD_TIMEOUT or SD_ERROR).
 */
int sd_read_block(unsigned int block_addr, unsigned char *buffer, unsigned int num_blocks);

/**
 * @brief Writes one or more blocks to the SD card from a provided buffer.
 *
 * This function performs a block-based write from the SD card starting at the specified block address.
 * It handles both high-capacity (block-addressed) and standard-capacity (byte-addressed) cards by checking
 * the card's capabilities and adjusting the command sequence accordingly.
 *
 * @param buffer     Pointer to the source buffer where the write is stored.
 * @param block_addr The starting block address to write to. 
 * @param num_blocks The number of blocks to write to the SD card.
 *
 * @return On success, returns the total number of bytes read (num_blocks * SD_BLK_SIZE).
 *         On failure, returns a negative error code (e.g., SD_TIMEOUT or SD_ERROR).
 */
int sd_write_block(unsigned char* buffer, unsigned int block_addr, unsigned int num_blocks);

/**
 * @brief Initialize SD card.
 */
int sd_init();
void sd_get_devices();

#endif
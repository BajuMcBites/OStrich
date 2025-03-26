#include "sdio_tests.h"

#include "printf.h"
#include "sdio.h"

/**
 * IMPORTANT: Block 0 is usually an important block so don't mess with it during tests.
 */

#define BLOCK_ADDR 10
#define NUM_BLOCKS 1

// Test the write function by writing a known pattern to the SD card.
void test_sdio_block_write() {
    unsigned char write_buf[SD_BLK_SIZE];

    // Fill the buffer with a simple pattern.
    for (unsigned int i = 0; i < SD_BLK_SIZE; i++) {
        write_buf[i] = (unsigned char)(i % 256);
    }

    int ret = sd_write_block(write_buf, BLOCK_ADDR, NUM_BLOCKS);
    if (ret < 0) {
        printf("test_sdio_block_write(): sd_write_block failed with error %d\n", ret);
    } else {
        printf("test_sdio_block_write(): passed\n");
    }
}

// Test the read function by reading the block and comparing it to the expected pattern.
void test_sdio_block_read() {
    unsigned char expected_buf[SD_BLK_SIZE];
    unsigned char read_buf[SD_BLK_SIZE];

    // Prepare the expected pattern (must match what was written in the write test).
    for (unsigned int i = 0; i < SD_BLK_SIZE; i++) {
        expected_buf[i] = (unsigned char)(i % 256);
    }

    int ret = sd_read_block(BLOCK_ADDR, read_buf, NUM_BLOCKS);
    if (ret < 0) {
        printf("test_sdio_block_read(): sd_read_block failed with error %d\n", ret);
        return;
    }

    // Compare each byte of the read data with the expected pattern.
    for (unsigned int i = 0; i < SD_BLK_SIZE; i++) {
        if (read_buf[i] != expected_buf[i]) {
            printf("test_sdio_block_read(): mismatch at index %d (expected %d, got %d)\n", i,
                   expected_buf[i], read_buf[i]);
            return;
        }
    }
    printf("test_sdio_block_read(): passed\n");
}

// Runs all SD card tests.
void sdioTests() {
    // Make sure to run these together.
    test_sdio_block_write();
    test_sdio_block_read();
}
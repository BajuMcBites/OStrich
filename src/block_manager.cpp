#include "../filesystem/interface/BlockManager.h"
#include "libk.h"
#include "partition.h"
#include "printf.h"
#include "sdio.h"

namespace fs {

// Hooks into filesystem interface written by Caleb and Ameya.
// Blocks are 4096 bytes (logical), sectors are 512 bytes (physical).
// FS thinks in terms of blocks, SD card thinks in terms of sectors (but SD terminology actually
// calls it a block as well).

BlockManager::BlockManager(int numSectors, int startSector)
    : numSectors(numSectors), startSector(startSector) {
    if (numSectors <= 0) {
        printf("Invalid number of sectors %d\n", numSectors);
        K::assert(false, "");
    }

    sectorsPerBlock = BLOCK_SIZE / SD_BLK_SIZE;  // should be 8.
    if (numSectors % sectorsPerBlock != 0) {
        printf("FS partition size %ld bytes is not a multiple of FS block size %d bytes\n",
               numSectors * SD_BLK_SIZE, BLOCK_SIZE);
        K::assert(false, "");
    }
    numBlocks = numSectors / sectorsPerBlock;
}

uint64_t BlockManager::blockToSectorIndex(size_t blockIndex) {
    return blockIndex * sectorsPerBlock + startSector;
}

bool BlockManager::isInvalidSectorIndex(size_t sectorIndex) {
    return sectorIndex < startSector || sectorIndex >= startSector + numSectors;
}

bool BlockManager::readBlock(size_t blockIndex, uint8_t* buffer) {
    int sectorIndex = blockToSectorIndex(blockIndex);
    if (isInvalidSectorIndex(sectorIndex)) {
        printf("Invalid block index %d, sector index %d\n", blockIndex, sectorIndex);
        return false;
    }
    // printf("Reading block %d, sector index %d\n", blockIndex, sectorIndex);
    int result = sd_read_block(sectorIndex, (unsigned char*)buffer, sectorsPerBlock);
    return result > 0;
}

bool BlockManager::writeBlock(size_t blockIndex, const uint8_t* buffer) {
    int sectorIndex = blockToSectorIndex(blockIndex);
    if (isInvalidSectorIndex(sectorIndex)) {
        printf("Invalid block index %d, sector index %d\n", blockIndex, sectorIndex);
        return false;
    }
    // printf("Writing block %d, sector index %d\n", blockIndex, sectorIndex);
    int result = sd_write_block((unsigned char*)buffer, sectorIndex, sectorsPerBlock);
    return result > 0;
}

}  // namespace fs
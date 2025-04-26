#ifndef BLOCK_MANAGER_H
#define BLOCK_MANAGER_H

#ifdef NOT_KERNEL
#include "FakeDiskDriver.h"
#include <vector>
#include <iostream>
#include <algorithm>
#include <mutex>
#endif

#include "cstdint"

namespace fs {

class BlockManager
{
public:
    // The file system block size is 4096 bytes.
    static constexpr size_t BLOCK_SIZE = 4096;
    // using BlockBuffer = uint8_t[BLOCK_SIZE];

    /**
     * Constructor.
     * @param disk       Reference to the underlying FakeDiskDriver.
     * @param partition  The partition (a slice of the disk) that this BlockManager will manage.
     */
    #ifdef NOT_KERNEL
    BlockManager(FakeDiskDriver& disk, const FakeDiskDriver::Partition& partition, int numBlocks);
    #else
    BlockManager(int numSectors, int startSector);
    #endif

    /**
     * Reads a file system block (4096 bytes) from the partition.
     * @param blockIndex Logical block index (0-based within the partition).
     * @param buffer      Output buffer (will be resized to BLOCK_SIZE).
     * @return true if the block was read successfully.
     */
    bool readBlock(size_t blockIndex, uint8_t* buffer);

    /**
     * Writes a file system block (4096 bytes) to the partition.
     * @param blockIndex Logical block index (0-based within the partition).
     * @param buffer      Input buffer of size BLOCK_SIZE.
     * @return true if the block was written successfully.
     */
    bool writeBlock(size_t blockIndex, const uint8_t* buffer);

    uint32_t getNumBlocks() const
    {
        return numBlocks;
    }

    #ifndef NOT_KERNEL
    uint64_t blockToSectorIndex(size_t blockIndex);
    bool isInvalidSectorIndex(size_t sectorIndex);
    #endif

private:
    #ifdef NOT_KERNEL
    FakeDiskDriver& disk;
    FakeDiskDriver::Partition partition;
    mutable std::mutex blockMutex; // Protects BlockManager state and operations.
    #endif
    int numBlocks;
    int numSectors;
    int startSector;
    size_t sectorsPerBlock; // Number of 512-byte sectors per 4096-byte block.
};

} // namespace fs

#endif // BLOCK_MANAGER_H

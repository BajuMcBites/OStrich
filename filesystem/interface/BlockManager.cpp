#include "BlockManager.h"

namespace fs {

BlockManager::BlockManager(FakeDiskDriver& disk, const FakeDiskDriver::Partition& partition, int numBlocks)
    : disk(disk), partition(partition), numBlocks(numBlocks)
{
    // Calculate the number of sectors per block.
    sectorsPerBlock = BLOCK_SIZE / FakeDiskDriver::SECTOR_SIZE;
    if (BLOCK_SIZE % FakeDiskDriver::SECTOR_SIZE != 0)
    {
        std::cerr << "Error: BLOCK_SIZE (" << BLOCK_SIZE
            << ") is not a multiple of the sector size ("
            << FakeDiskDriver::SECTOR_SIZE << ").\n";
    }
}

bool BlockManager::readBlock(const size_t blockIndex, uint8_t* buffer)
{
    // std::cout << "\tReading block " << blockIndex << "\n";
    std::lock_guard<std::mutex> lock(blockMutex);
    size_t startSector = partition.startSector + blockIndex * sectorsPerBlock;
    if (blockIndex * sectorsPerBlock >= partition.sectorCount)
    {
        std::cerr << "readBlock: block index " << blockIndex << " is out of partition range.\n";
        return false;
    }

    // block.resize(BLOCK_SIZE);
    for (size_t i = 0; i < sectorsPerBlock; i++)
    {
        if (!disk.readSector(startSector + i, buffer + i * FakeDiskDriver::SECTOR_SIZE))
        {
            std::cerr << "readBlock: failed to read sector " << (startSector + i) << "\n";
            return false;
        }
    }
    return true;
}

bool BlockManager::writeBlock(const size_t blockIndex, const uint8_t* buffer)
{
    // std::cout << "\tWriting block " << blockIndex << "\n";
    std::lock_guard<std::mutex> lock(blockMutex);
    // if (block.size() != BLOCK_SIZE) {
    //     std::cerr << "writeBlock: block size mismatch (expected " << BLOCK_SIZE << " bytes).\n";
    //     return false;
    // }

    size_t startSector = partition.startSector + blockIndex * sectorsPerBlock;
    if (blockIndex * sectorsPerBlock >= partition.sectorCount)
    {
        std::cerr << "writeBlock: block index " << blockIndex << " is out of partition range.\n";
        return false;
    }

    for (size_t i = 0; i < sectorsPerBlock; i++)
    {
        if (!disk.writeSector(startSector + i, buffer + i * FakeDiskDriver::SECTOR_SIZE))
        {
            std::cerr << "writeBlock: failed to write sector " << (startSector + i) << "\n";
            return false;
        }
    }
    return true;
}

} // namespace fs
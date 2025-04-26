//
// Created by ameya on 2/21/2025.
//

#include "InodeTable.h"
#include "cstdio"
#include "LogManager.h"
#include "LogRecord.h"

namespace fs {

InodeTable::InodeTable(const block_index_t startBlock, const inode_index_t numBlocks, const inode_index_t size, const inode_index_t inodeRegionStart,
                       BlockManager* blockManager): startBlock(startBlock), numBlocks(numBlocks), size(size), inodeRegionStart(inodeRegionStart),
                                                    blockManager(blockManager)
{
}

bool InodeTable::initialize(const block_index_t startBlock, const inode_index_t numBlocks,
                            BlockManager* blockManager)
{
    block_t tempBlock;
    for (inode_index_t i = 0; i < numBlocks; i++)
    {
        for (inode_index_t j = 0; j < TABLE_ENTRIES_PER_BLOCK; j++)
        {
            tempBlock.inodeTable.inodeNumbers[j] = INODE_NULL_VALUE;
        }
        if (!blockManager->writeBlock(startBlock + i, tempBlock.data))
        {
            printf("Could not write inode table block\n");
            return false;
        }
    }
    return true;
}

inode_index_t InodeTable::getFreeInodeNumber()
{
    block_t tempBlock;
    for (inode_index_t i = 0; i < size; i++)
    {
        if (i % TABLE_ENTRIES_PER_BLOCK == 0)
        {
            if (!blockManager->readBlock(startBlock + i / TABLE_ENTRIES_PER_BLOCK, tempBlock.data))
            {
                printf("Could not read inode table block\n");
                return INODE_NULL_VALUE;
            }
        }
        if (tempBlock.inodeTable.inodeNumbers[i % TABLE_ENTRIES_PER_BLOCK] == INODE_NULL_VALUE)
        {
            return i;
        }
    }
    return INODE_NULL_VALUE;
}

bool InodeTable::setInodeLocation(inode_index_t inodeNumber, inode_index_t location)
{
    if (inodeNumber >= size)
    {
        printf("Inode number out of bounds\n");
        return false;
    }
    if (snapshotMode) {
        // Update the native array in snapshot mode.
        snapshotMapping[inodeNumber] = location;
        return true;
    }
    inode_index_t blockNum = inodeNumber / TABLE_ENTRIES_PER_BLOCK;
    inode_index_t entryNum = inodeNumber % TABLE_ENTRIES_PER_BLOCK;
    block_t tempBlock;
    if (!blockManager->readBlock(startBlock + blockNum, tempBlock.data))
    {
        printf("Could not read inode table block\n");
        return false;
    }
    tempBlock.inodeTable.inodeNumbers[entryNum] = location;
    if (!blockManager->writeBlock(startBlock + blockNum, tempBlock.data))
    {
        printf("Could not write inode table block\n");
        return false;
    }
    return true;
}

inode_index_t InodeTable::getInodeLocation(inode_index_t inodeNumber)
{
    if (inodeNumber >= size)
    {
        printf("Inode number out of bounds\n");
        return INODE_NULL_VALUE;
    }
    if (snapshotMode) {
        // Return from the native array.
 //       cout << "Got snapshot Inode mapping: " << inodeNumber << " to " << snapshotMapping[inodeNumber] << endl;
        return snapshotMapping[inodeNumber];
    }
    inode_index_t blockNum = inodeNumber / TABLE_ENTRIES_PER_BLOCK;
    inode_index_t entryNum = inodeNumber % TABLE_ENTRIES_PER_BLOCK;
    block_t tempBlock;
    if (!blockManager->readBlock(startBlock + blockNum, tempBlock.data))
    {
        printf("Could not read inode table block\n");
        return INODE_NULL_VALUE;
    }
    return tempBlock.inodeTable.inodeNumbers[entryNum];
}

bool InodeTable::writeInode(inode_index_t inodeLocation, inode_t& inode)
{
    block_index_t inodeBlock = inodeRegionStart + inodeLocation / INODES_PER_BLOCK;
    block_t tempBlock;
    if (!blockManager->readBlock(inodeBlock, tempBlock.data))
    {
        printf("Could not read inode block\n");
        return false;
    }
    tempBlock.inodeBlock.inodes[inodeLocation % INODES_PER_BLOCK] = inode;
    if (!blockManager->writeBlock(inodeBlock, tempBlock.data))
    {
        printf("Could not write inode block\n");
        return false;
    }
    // std::cout << "[write inode] inodeLocation: " << inodeLocation << std::endl;
    // std::cout << "\tpermissions: " << inode.permissions << std::endl;
    return true;
}

bool InodeTable::readInode(inode_index_t inodeLocation, inode_t& inode)
{
    block_index_t inodeBlock = inodeRegionStart + inodeLocation / INODES_PER_BLOCK;
    block_t tempBlock;
    if (!blockManager->readBlock(inodeBlock, tempBlock.data))
    {
        printf("Could not read inode block\n");
        return false;
    }
    inode = tempBlock.inodeBlock.inodes[inodeLocation % INODES_PER_BLOCK];
    // std::cout << "[read inode] inodeLocation: " << inodeLocation << std::endl;
    return true;
}

bool InodeTable::readInodeBlock(inode_index_t blockIndex, inode_index_t* outBuffer)
{
    // Ensure blockIndex is within the number of inode table blocks.
    if (blockIndex >= numBlocks) {
        printf("readInodeBlock: block index %d is out of range.\n", blockIndex);
        return false;
    }
    block_t tempBlock;
    // Read the block from disk. The inode table blocks start at startBlock.
    if (!blockManager->readBlock(startBlock + blockIndex, tempBlock.data)) {
        printf("readInodeBlock: could not read inode table block %d\n", (startBlock + blockIndex));
        return false;
    }
    // Copy the entire array of inodeNumbers from the block into outBuffer.
    memcpy(outBuffer, tempBlock.inodeTable.inodeNumbers, TABLE_ENTRIES_PER_BLOCK * sizeof(inode_index_t));
    return true;
}

InodeTable* InodeTable::createSnapshotFromCheckpoint(block_index_t checkpointBlockIndex, InodeTable* liveTable)
{
    // Create a new InodeTable instance using the live table's parameters.
    auto* snapshot = new InodeTable(liveTable->startBlock, liveTable->numBlocks, liveTable->size,
                                           liveTable->inodeRegionStart, liveTable->blockManager);
    // Set snapshot mode and allocate the native array.
    snapshot->snapshotMode = true;
    snapshot->snapshotMapping = new inode_index_t[liveTable->size];
    for (inode_index_t i = 0; i < liveTable->size; i++) {
        snapshot->snapshotMapping[i] = INODE_NULL_VALUE;
    }

    inode_index_t totalInodes = liveTable->size;

    // Traverse the checkpoint chain to set the mapping entries.
    checkpointBlock_t checkpoint;
    block_index_t currentCp = checkpointBlockIndex;
    while (true) {
        if (!liveTable->blockManager->readBlock(currentCp, reinterpret_cast<uint8_t*>(&checkpoint))) {
            printf("Snapshot: Failed to read checkpoint block at %d\n", currentCp);
            delete snapshot;
            return nullptr;
        }
        if (checkpoint.magic != CHECKPOINT_MAGIC) {
            printf("Snapshot: Invalid checkpoint magic at %d\n", currentCp);
            delete snapshot;
            return nullptr;
        }
        for (uint32_t i = 0; i < checkpoint.numEntries; i++) {
            inode_index_t idx = checkpoint.entries[i].inodeIndex;
            inode_index_t cpLocation = checkpoint.entries[i].inodeLocation;
            if (idx < totalInodes) {
//                cout << "Setting snapshot mapping for inode index " << idx << " to location " << cpLocation << endl;
                snapshot->snapshotMapping[idx] = cpLocation;
            }
        }
        if (checkpoint.nextCheckpointBlock == NULL_INDEX)
            break;
        currentCp = checkpoint.nextCheckpointBlock;
    }

    return snapshot;
}

} // namespace fs
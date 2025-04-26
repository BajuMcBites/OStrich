//
// Created by ameya on 2/20/2025.
//

#include "FileSystem.h"

#include "cstring"
#include "cstdio"
#include "cassert"


#include "Directory.h"

namespace fs {

static const block_index_t LOG_AREA_SIZE = 64; // Reserve 64 blocks for the log area, need to adjust this later
static const uint32_t NUM_CHECKPOINTS = 128;

// Do not remove.
FileSystem* FileSystem::instance = nullptr;

FileSystem* FileSystem::getInstance(BlockManager* blockManager) {
    if (!instance) {
        instance = new FileSystem(blockManager);
    }
    return instance;
}

FileSystem::FileSystem(BlockManager* blockManager): blockManager(blockManager), inodeBitmap(nullptr),
                                                    blockBitmap(nullptr)
{
    // Check if blockManager is nullptr.
    if (!blockManager) {
        printf("Block manager is nullptr\n");
        assert(0);
    }

    this->superBlock = &superBlockWrapper.superBlock;
    if (!blockManager->readBlock(0, superBlockWrapper.data))
    {
        printf("Could not read superblock\n");
        assert(0);
    }

    if (superBlock->magic != MAGIC_NUMBER)
    {
         printf("Creating new filesystem; found magic: %d | expected: %d\n", superBlock->magic, MAGIC_NUMBER);
        createFilesystem();
    }
    else
    {
        printf("Existing filesystem detected\n");
    }
    loadFilesystem();
}

Directory* FileSystem::getRootDirectory() const
{
    return new Directory(0, inodeTable, inodeBitmap, blockBitmap, blockManager, logManager);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void FileSystem::createFilesystem()
{
    superBlock->magic = MAGIC_NUMBER;
    superBlock->totalBlockCount = blockManager->getNumBlocks() - 1;
    superBlock->inodeCount = 4 * (superBlock->totalBlockCount / 16);
    superBlock->inodeBitmapSize = ((superBlock->inodeCount + 7) / 8 + BlockManager::BLOCK_SIZE - 1)
        /
        BlockManager::BLOCK_SIZE;
    superBlock->inodeRegionSize = (superBlock->inodeCount * sizeof(inode_t) +
            BlockManager::BLOCK_SIZE - 1) /
        BlockManager::BLOCK_SIZE;
    superBlock->inodeTableSize = (superBlock->inodeCount * sizeof(inode_index_t) +
            BlockManager::BLOCK_SIZE - 1) /
        BlockManager::BLOCK_SIZE;

    // Calculate remaining blocks for data and log area.
    const block_index_t remainingBlocks = superBlock->totalBlockCount - superBlock->inodeBitmapSize -
        superBlock->inodeRegionSize - superBlock->inodeTableSize;
    superBlock->dataBlockBitmapSize = ((remainingBlocks + 7) / 8 + BlockManager::BLOCK_SIZE - 1) /
        BlockManager::BLOCK_SIZE;

    // Reserve LOG_AREA_SIZE blocks for logging.
    if (remainingBlocks < superBlock->dataBlockBitmapSize + LOG_AREA_SIZE)
    {
        printf("Not enough blocks remaining for data and log areas.\n");
        assert(0);
    }
    superBlock->dataBlockCount = remainingBlocks - superBlock->dataBlockBitmapSize - LOG_AREA_SIZE;
    superBlock->freeDataBlockCount = superBlock->dataBlockCount;
    superBlock->freeInodeCount = superBlock->inodeCount;
    superBlock->size = superBlock->dataBlockCount * BlockManager::BLOCK_SIZE;

    // Layout: bitmaps and tables are laid out consecutively.
    superBlock->inodeBitmap = 1;
    superBlock->inodeTable = superBlock->inodeBitmap + superBlock->inodeBitmapSize;
    superBlock->dataBlockBitmap = superBlock->inodeTable + superBlock->inodeTableSize;
    superBlock->inodeRegionStart = superBlock->dataBlockBitmap + superBlock->dataBlockBitmapSize;
    superBlock->dataBlockRegionStart = superBlock->inodeRegionStart + superBlock->inodeRegionSize;

    // Reserve the log area at the end of the disk.
    superBlock->logAreaStart = superBlock->dataBlockRegionStart + superBlock->dataBlockCount;
    superBlock->logAreaSize = LOG_AREA_SIZE;

    // Initialize other fields.
    superBlock->systemStateSeqNum = 0;
    superBlock->latestCheckpointIndex = 0;
    for (int i = 0; i < NUM_CHECKPOINTS; i++)
    {
        superBlock->checkpointArr[i] = 0;
    }


    // Zero out the bitmaps.
    constexpr block_t zeroBlock{};
    for (block_index_t i = 0; i < superBlock->dataBlockBitmapSize; i++)
    {
        if (!blockManager->writeBlock(superBlock->dataBlockBitmap + i, zeroBlock.data))
        {
            printf("Could not write block bitmap\n");
            assert(0);
        }
    }

    for (block_index_t i = 0; i < superBlock->inodeBitmapSize; i++)
    {
        if (!blockManager->writeBlock(superBlock->inodeBitmap + i, zeroBlock.data))
        {
            printf("Could not write inode bitmap\n");
            assert(0);
        }
    }

    InodeTable::initialize(superBlock->inodeTable, superBlock->inodeTableSize, blockManager);

    if (!blockManager->writeBlock(0, superBlockWrapper.data))
    {
        printf("Could not write superblock\n");
        assert(0);
    }
}

void FileSystem::loadFilesystem()
{
    // std::cout << "Size: " << superBlock->size << std::endl;
    // std::cout << "Inode region start: " << superBlock->inodeRegionStart << std::endl;
    // std::cout << "Data block region start: " << superBlock->dataBlockRegionStart << std::endl;
    inodeBitmap = new BitmapManager(superBlock->inodeBitmap, superBlock->inodeBitmapSize, superBlock->inodeCount,
                                    blockManager);
    blockBitmap = new BitmapManager(superBlock->dataBlockBitmap, superBlock->dataBlockBitmapSize,
                                    superBlock->dataBlockCount, blockManager, superBlock->dataBlockRegionStart);
    inodeTable = new InodeTable(superBlock->inodeTable, superBlock->inodeTableSize, superBlock->inodeCount,
                                superBlock->inodeRegionStart,
                                blockManager);

    // Initialize LogManager using the log area from the superblock.
    logManager = new LogManager(blockManager, blockBitmap, inodeTable, superBlock->logAreaStart, superBlock->logAreaSize, superBlock->systemStateSeqNum);
    if (inodeTable->getInodeLocation(0) == INODE_NULL_VALUE)
    {
        delete createRootInode();
    }

}

bool FileSystem::readInode(inode_index_t inodeLocation, inode_t& inode)
{
    block_index_t inodeBlock = superBlock->inodeRegionStart + inodeLocation / INODES_PER_BLOCK;
    block_t tempBlock;
    if (!blockManager->readBlock(inodeBlock, tempBlock.data))
    {
        printf("Could not read inode block\n");
        return false;
    }
    inode = tempBlock.inodeBlock.inodes[inodeLocation % INODES_PER_BLOCK];
    return true;
}

bool FileSystem::writeInode(inode_index_t inodeLocation, inode_t& inode)
{
    block_index_t inodeBlock = superBlock->inodeRegionStart + inodeLocation / INODES_PER_BLOCK;
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
    return true;
}

Directory* FileSystem::createRootInode()
{
     printf("Creating root inode\n");
    return new Directory(inodeTable, inodeBitmap, blockBitmap, blockManager, logManager, DIRECTORY_MASK);
}

bool FileSystem::createCheckpoint() {
    return logManager->createCheckpoint();
}

// Modified mountReadOnlySnapshot using the new snapshot functionality.
FileSystem* FileSystem::mountReadOnlySnapshot(uint32_t checkpointID) {
    // Read the superblock from disk.
    block_t superBlockTemp;
    if (!blockManager->readBlock(0, superBlockTemp.data)) {
        printf("mountReadOnlySnapshot: Failed to read superblock\n");
        return nullptr;
    }
    // Validate checkpointID and obtain the checkpoint block index.
    if (checkpointID >= 128) {
        printf("mountReadOnlySnapshot: Invalid checkpointID\n");
        return nullptr;
    }
    block_index_t cpBlock = superBlockTemp.superBlock.checkpointArr[checkpointID];
    if (cpBlock == 0) {
        printf("mountReadOnlySnapshot: Checkpoint not available\n");
        return nullptr;
    }
    // Create a snapshot of the inode table from the checkpoint chain.
    InodeTable* snapshotInodeTable = InodeTable::createSnapshotFromCheckpoint(cpBlock, this->inodeTable);
    if (!snapshotInodeTable) {
        printf("mountReadOnlySnapshot: Failed to create snapshot inode table\n");
        return nullptr;
    }
    // Create a new FileSystem instance (the live instance remains unchanged).
    FileSystem* snapshotFS = new FileSystem(blockManager);
    // Replace the live inode table with our snapshot version.
    delete snapshotFS->inodeTable; // Free the inode table loaded in the constructor.
    snapshotFS->inodeTable = snapshotInodeTable;
    // Mark the snapshot as read-only.
    snapshotFS->readOnly = true;
    // ToDo reject writes in the snapshotFS instance somehow (probably need to add flag checks).
    return snapshotFS;
}

} // namespace fs
//
// Created by ameya on 3/11/2025.
//

#include "Directory.h"

#include "cstring"
#include "cstdio"
#include "vector"

namespace fs {

Directory::Directory(InodeTable* inodeTable, BitmapManager* inodeBitmap, BitmapManager* blockBitmap,
                     BlockManager* blockManager, LogManager* logManager, const uint16_t permissions): File(
    inodeTable, inodeBitmap, blockBitmap, blockManager, logManager, permissions)
{
}

Directory::Directory(inode_index_t inodeNum, InodeTable* inodeTable, BitmapManager* inodeBitmap,
                     BitmapManager* blockBitmap, BlockManager* blockManager,  LogManager* logManager): File(
    inodeNum, inodeTable, inodeBitmap, blockBitmap, blockManager, logManager)
{
}

inode_index_t Directory::getDirectoryEntry(const char* fileName) const
{
    block_t block;
    for (inode_index_t i = 0; i < inode.blockCount; i++)
    {
        read_block_data(i, block.data);
        uint8_t offset = 0;
        for (auto [inodeNumber, name] : block.directoryBlock.entries)
        {
            if (strcmp(name, fileName) == 0)
            {
                // std::cout << "Returning inode number: " << inodeNumber << " for file: " << fileName << std::endl;
                return inodeNumber;
            }
            offset++;
            if (offset + i * DIRECTORY_ENTRIES_PER_BLOCK == inode.numFiles)
            {
                break;
            }
        }
    }
    return INODE_NULL_VALUE;
}

bool Directory::addDirectoryEntry(const char* fileName, inode_index_t fileNum)
{
    // Determine the offset in the last directory block based on the current number of entries.
    const uint16_t offset = inode.numFiles % DIRECTORY_ENTRIES_PER_BLOCK;

    // If offset is zero, either there are no entries yet or the previous block is full.
    if (offset == 0)
    {
        // --- Copy-on-write branch for a new directory block ---
//        std::cout << "DEBUG: Creating new directory block for first entry in directory inode " << getInodeNumber() << std::endl;
        // Allocate a fresh block for this new set of directory entries.
        block_t newBlock{};
        // Write the new entry into index 0.
        strncpy(newBlock.directoryBlock.entries[0].name, fileName, MAX_FILE_NAME_LENGTH);
        newBlock.directoryBlock.entries[0].name[MAX_FILE_NAME_LENGTH] = '\0';
        newBlock.directoryBlock.entries[0].inodeNumber = fileNum;

        // Update the directory metadata.
        inode.numFiles++;
//        std::cout << "DEBUG: Updated inode numFiles to " << inode.numFiles << " for directory inode " << getInodeNumber() << std::endl;
//        std::cout << "DEBUG: Calling write_new_block_data to allocate new directory block for inode " << getInodeNumber() << std::endl;
        // Allocate a new block for this directory block.
        if (!write_new_block_data(newBlock.data))
        {
            printf("Failed to allocate new directory block for entry: %s\n", fileName);
            return false;
        }
//        std::cout << "DEBUG: New directory block allocated and written for inode " << getInodeNumber() << std::endl;
        return true;
    }
    else
    {
        // --- Copy-on-write branch for updating an existing directory block ---
        // Read the last directory block.
        block_t oldBlock{};
        if (!read_block_data(inode.blockCount - 1, oldBlock.data))
        {
            printf("Failed to read current directory block\n");
            return false;
        }
//        std::cout << "DEBUG: Read old directory block from physical block " << inode.directBlocks[inode.blockCount - 1] << " for directory inode " << getInodeNumber() << std::endl;
        // Make a new copy of that block.
        block_t newBlock{};
        memcpy(newBlock.data, oldBlock.data, sizeof(newBlock.data));

        // Write the new directory entry into the new copy at the computed offset.
        strncpy(newBlock.directoryBlock.entries[offset].name, fileName, MAX_FILE_NAME_LENGTH);
        newBlock.directoryBlock.entries[offset].name[MAX_FILE_NAME_LENGTH] = '\0';
        newBlock.directoryBlock.entries[offset].inodeNumber = fileNum;

        // Update the total number of entries.
        inode.numFiles++;
//        std::cout << "DEBUG: Updated inode numFiles to " << inode.numFiles << " for directory inode " << getInodeNumber() << std::endl;
        // Allocate a new block for the updated directory block.
        block_index_t newBlockLocation = blockBitmap->findNextFree();
        if (newBlockLocation == BLOCK_NULL_VALUE)
        {
            printf("No free block available for copy-on-write directory update\n");
            return false;
        }
        if (!blockBitmap->setAllocated(newBlockLocation))
        {
            printf("Failed to mark new block as allocated\n");
            return false;
        }
//        std::cout << "DEBUG: Allocated new copy-on-write directory block at physical block " << newBlockLocation << " for directory inode " << getInodeNumber() << std::endl;

        // Write the new block data to disk.
        if (!blockManager->writeBlock(newBlockLocation, newBlock.data))
        {
            printf("Failed to write updated directory block to disk\n");
            return false;
        }
//        std::cout << "DEBUG: Wrote new directory block to disk at physical block " << newBlockLocation << " for directory inode " << getInodeNumber() << std::endl;


//        std::cout << "DEBUG: Updating inode table for directory inode " << getInodeNumber() << ": replacing block pointer " << inode.directBlocks[inode.blockCount - 1] << " with new block " << newBlockLocation << std::endl;
        // Update the parent's inode to reference the new block.
        // (We update the pointer for the last block in the directory.)
        inode.directBlocks[inode.blockCount - 1] = newBlockLocation;

        // --- Perform copy-on-write update on the parent's inode ---
        inode_index_t newInodeLocation = inodeBitmap->findNextFree();
        if (newInodeLocation == NULL_INDEX) {
            printf("Failed to allocate new inode for copy-on-write update\n");
            return false;
        }
        if (!inodeBitmap->setAllocated(newInodeLocation)) {
            printf("Failed to mark new inode as allocated\n");
            return false;
        }
        // Create a new parent's inode by copying the updated inode.
        inode_t newParentInode = inode;
        // Write the new inode to disk.
        if (!inodeTable->writeInode(newInodeLocation, newParentInode)) {
            printf("Failed to write updated parent's inode to disk\n");
            return false;
        }
        // Log the parent's inode update.
        {
            LogRecordPayload payload{};
            payload.inodeUpdate.inodeIndex = getInodeNumber();
            payload.inodeUpdate.inodeLocation = newInodeLocation;
            if (!logManager->logOperation(LogOpType::LOG_OP_INODE_UPDATE, &payload)) {
                printf("Failed to log parent's inode update\n");
                return false;
            }
        }
        // Update the inode table to point to the new parent's inode location.
        if (!inodeTable->setInodeLocation(getInodeNumber(), newInodeLocation)) {
            printf("Failed to update inode table for parent's inode\n");
            return false;
        }
        return true;
    }
}


bool Directory::removeDirectoryEntry(const char* fileName) {


    // Iterate over each block in the directory's inode block list.
    block_t block;
    for (uint32_t i = 0; i < inode.blockCount; i++) {
        // Read the current directory block.
        if (!read_block_data(i, block.data)) {
            printf("Failed to read directory block %d\n", i);
            continue;
        }
        // Search through all entries in this block.
        for (uint16_t j = 0; j < DIRECTORY_ENTRIES_PER_BLOCK; j++) {
            // Check if the entry is valid and matches the file name.
            if (block.directoryBlock.entries[j].inodeNumber != INODE_NULL_VALUE &&
                strcmp(block.directoryBlock.entries[j].name, fileName) == 0) {

                // Calculate the global index of the found entry.
                uint32_t entryGlobalIndex = i * DIRECTORY_ENTRIES_PER_BLOCK + j;
                if (entryGlobalIndex >= inode.numFiles) {
                    printf("Entry global index out of bounds\n");
                    return false;
                }

                // Log the deletion of the inode by creating a deletion log record.
                LogRecordPayload payload{};
                payload.inodeDelete.inodeIndex = block.directoryBlock.entries[j].inodeNumber;
                if (!logManager->logOperation(LogOpType::LOG_OP_INODE_DELETE, &payload)) {
                    printf("Failed to log inode deletion\n");
                    return false;
                }

                // Prepare a new copy of the directory block (copy-on-write).
                block_t newBlock;
                memcpy(newBlock.data, block.data, sizeof(block.data));

                // Determine the last valid directory entry (global index = inode.numFiles - 1).
                uint32_t lastEntryGlobalIndex = inode.numFiles - 1;
                uint32_t lastBlockIndex = lastEntryGlobalIndex / DIRECTORY_ENTRIES_PER_BLOCK;
                uint16_t lastOffset = lastEntryGlobalIndex % DIRECTORY_ENTRIES_PER_BLOCK;

                if (entryGlobalIndex != lastEntryGlobalIndex) {
                    // If the entry being deleted is not the last one, swap it with the last entry.
                    if (lastBlockIndex == i) {
                        // Both entries are in the same block.
                        newBlock.directoryBlock.entries[j] = newBlock.directoryBlock.entries[lastOffset];
                    } else {
                        // The last entry is in a different block. Read that block.
                        block_t lastBlock;
                        if (!read_block_data(lastBlockIndex, lastBlock.data)) {
                            printf("Failed to read last directory block\n");
                            return false;
                        }
                        newBlock.directoryBlock.entries[j] = lastBlock.directoryBlock.entries[lastOffset];
                        // Remove the last entry from its block copy by clearing it.
                        memset(lastBlock.directoryBlock.entries[lastOffset].name, 0, MAX_FILE_NAME_LENGTH + 1);
                        lastBlock.directoryBlock.entries[lastOffset].inodeNumber = INODE_NULL_VALUE;
                        // Perform copy-on-write update for the last block.
                        block_index_t newLastBlock = blockBitmap->findNextFree();
                        if (newLastBlock == BLOCK_NULL_VALUE) {
                            printf("No free block for copy-on-write update of last block\n");
                            return false;
                        }
                        if (!blockBitmap->setAllocated(newLastBlock)) {
                            printf("Failed to set allocated for new last block\n");
                            return false;
                        }
                        if (!blockManager->writeBlock(newLastBlock, lastBlock.data)) {
                            printf("Failed to write updated last block\n");
                            return false;
                        }
                        // Update the directory inode pointer for the last block.
                        inode.directBlocks[lastBlockIndex] = newLastBlock;
                    }
                } else {
                    // If the entry to remove is the last one, simply clear it in the new copy.
                    memset(newBlock.directoryBlock.entries[j].name, 0, MAX_FILE_NAME_LENGTH + 1);
                    newBlock.directoryBlock.entries[j].inodeNumber = INODE_NULL_VALUE;
                }

                // Decrement the total number of directory entries.
                inode.numFiles--;

                // Now, perform a copy-on-write update for the directory block where deletion occurred.
                block_index_t newBlockLocation = blockBitmap->findNextFree();
                if (newBlockLocation == BLOCK_NULL_VALUE) {
                    printf("No free block available for copy-on-write directory update\n");
                    return false;
                }
                if (!blockBitmap->setAllocated(newBlockLocation)) {
                    printf("Failed to set allocated for new directory block\n");
                    return false;
                }
                if (!blockManager->writeBlock(newBlockLocation, newBlock.data)) {
                    printf("Failed to write new directory block\n");
                    return false;
                }
                // Update the directory inode pointer to reference the new copy.
                inode.directBlocks[i] = newBlockLocation;

                // --- Perform copy-on-write update on the parent's inode ---
                inode_index_t newInodeLocation = inodeBitmap->findNextFree();
                if (newInodeLocation == NULL_INDEX) {
                    printf("Failed to allocate new inode for copy-on-write update in removeDirectoryEntry\n");
                    return false;
                }
                if (!inodeBitmap->setAllocated(newInodeLocation)) {
                    printf("Failed to mark new inode as allocated in removeDirectoryEntry\n");
                    return false;
                }
                // Create a new parent's inode by copying the updated inode.
                inode_t newParentInode = inode;
                // Write the new inode to disk.
                if (!inodeTable->writeInode(newInodeLocation, newParentInode)) {
                    printf("Failed to write updated parent's inode to disk in removeDirectoryEntry\n");
                    return false;
                }
                // Log the parent's inode update.
                {
                    LogRecordPayload payload{};
                    payload.inodeUpdate.inodeIndex = getInodeNumber();
                    payload.inodeUpdate.inodeLocation = newInodeLocation;
                    if (!logManager->logOperation(LogOpType::LOG_OP_INODE_UPDATE, &payload)) {
                        printf("Failed to log parent's inode update in removeDirectoryEntry\n");
                        return false;
                    }
                }
                // Update the inode table to point to the new parent's inode location.
                if (!inodeTable->setInodeLocation(getInodeNumber(), newInodeLocation)) {
                    printf("Failed to update inode table for parent's inode in removeDirectoryEntry\n");
                    return false;
                }
                return true;

            }
        }
    }
    // If no matching entry is found, return false.
    return false;
}

bool Directory::modifyDirectoryEntry(const char* fileName, inode_index_t fileNum)
{
    block_t block;
    for (inode_index_t i = 0; i < inode.blockCount; i++)
    {
        read_block_data(i, block.data);
        uint8_t offset = 0;
        for (auto [inodeNumber, name] : block.directoryBlock.entries)
        {
            if (strcmp(name, fileName) == 0)
            {
                block.directoryBlock.entries[offset].inodeNumber = fileNum;
                return write_block_data(i, block.data);
            }
            offset++;
        }
    }
    return false;
}

std::vector<char*> Directory::listDirectoryEntries() const
{
    std::vector<char*> entries(inode.numFiles);
    block_t block;
    inode_index_t cur = 0;
    for (inode_index_t i = 0; i < inode.blockCount; i++)
    {
        read_block_data(i, block.data);
        for (auto [inodeNumber, name] : block.directoryBlock.entries)
        {
            if (cur == inode.numFiles)
            {
                return entries;
            }
            entries[cur] = new char[MAX_FILE_NAME_LENGTH + 1];
            strncpy(entries[cur], name, MAX_FILE_NAME_LENGTH);
            entries[cur][MAX_FILE_NAME_LENGTH] = '\0';
            cur++;
        }
    }
    return entries;
}

Directory* Directory::createDirectory(const char* name)
{
    auto* newDir = new Directory(inodeTable, inodeBitmap, blockBitmap, blockManager, logManager);
    if (!addDirectoryEntry(name, newDir->getInodeNumber()))
    {
        return nullptr;
    }
    return newDir;
}

File* Directory::createFile(const char* name)
{
    auto* newFile = new File(inodeTable, inodeBitmap, blockBitmap, blockManager, logManager);
    if (!addDirectoryEntry(name, newFile->getInodeNumber()))
    {
        return nullptr;
    }
    return newFile;
}

File* Directory::getFile(const char* name) const
{
    const inode_index_t inodeNumber = getDirectoryEntry(name);
    if (inodeNumber == INODE_NULL_VALUE)
    {
        return nullptr;
    }
    File* file = new File(inodeNumber, inodeTable, inodeBitmap, blockBitmap, blockManager, logManager);
    if (file->isDirectory())
    {
        return convertFile(file);
    }
    return file;
}

Directory::Directory(const File* file) : File(file)
{
}

Directory* Directory::convertFile(File* file)
{
    if (!file->isDirectory()) return nullptr;
    auto* dir = new Directory(file);
    delete file;
    return dir;
}

} // namespace fs

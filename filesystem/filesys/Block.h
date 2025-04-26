//
// Created by ameya on 2/19/2025.
//

#ifndef BLOCK_H
#define BLOCK_H
#include "cstdint"
#include "../interface/BlockManager.h"
#include "LogRecord.h"

namespace fs {

using block_index_t = uint32_t;
using inode_index_t = uint32_t;
using block_offset_t = uint16_t;

constexpr uint64_t MAGIC_NUMBER = 0xCA5CADEDBA5EBA11;
constexpr block_index_t BLOCK_NULL_VALUE = UINT32_MAX;
constexpr inode_index_t INODE_NULL_VALUE = UINT32_MAX;

constexpr uint16_t NUM_DIRECT_BLOCKS = 15;
constexpr uint16_t NUM_INDIRECT_BLOCKS = 10;
constexpr uint16_t NUM_DOUBLE_INDIRECT_BLOCKS = 2;
constexpr uint16_t NUM_CHECKPOINTENTRIES_PER_CHECKPOINT = 504;

constexpr uint16_t DIRECTORY_MASK = 1 << 9;

typedef struct inode
{
    uint64_t size;
    block_index_t blockCount;
    uint16_t uid;
    uint16_t gid;
    uint16_t permissions;
    uint16_t numFiles;
    block_index_t directBlocks[NUM_DIRECT_BLOCKS];
    block_index_t indirectBlocks[NUM_INDIRECT_BLOCKS];
    block_index_t doubleIndirectBlocks[NUM_DOUBLE_INDIRECT_BLOCKS];
} inode_t;

constexpr uint16_t INODES_PER_BLOCK = BlockManager::BLOCK_SIZE / sizeof(inode_t);

typedef struct inodeBlock
{
    inode_t inodes[INODES_PER_BLOCK];
} inodeBlock_t;

typedef struct directBlock
{
    block_index_t blockNumbers[1024];
} directBlock_t;


typedef struct superBlock
{
    uint64_t magic;
    uint64_t size;
    block_index_t totalBlockCount;
    block_index_t dataBlockCount;
    inode_index_t inodeCount;
    block_index_t freeDataBlockCount;
    inode_index_t freeInodeCount;
    block_index_t dataBlockBitmap;
    block_index_t dataBlockBitmapSize;
    block_index_t inodeBitmap;
    inode_index_t inodeBitmapSize;
    block_index_t inodeTable;
    inode_index_t inodeTableSize;
    block_index_t dataBlockRegionStart;
    block_index_t inodeRegionStart;
    inode_index_t inodeRegionSize;
    block_index_t logAreaStart;
    block_index_t logAreaSize;
    uint64_t systemStateSeqNum;
    uint16_t latestCheckpointIndex;
    block_index_t checkpointArr[128];
    bool readOnly;
} superBlock_t;

typedef struct bitmapBlock
{
    uint64_t parts[512];
} bitmapBlock_t;

constexpr uint16_t TABLE_ENTRIES_PER_BLOCK = BlockManager::BLOCK_SIZE / sizeof(inode_index_t);

typedef struct inodeTableBlock
{
    inode_index_t inodeNumbers[TABLE_ENTRIES_PER_BLOCK];
} inodeTableBlock_t;

constexpr uint16_t MAX_FILE_NAME_LENGTH = 251;

typedef struct dirEntry
{
    inode_index_t inodeNumber;
    char name[MAX_FILE_NAME_LENGTH+1];
} dirEntry_t;

constexpr uint16_t DIRECTORY_ENTRIES_PER_BLOCK = BlockManager::BLOCK_SIZE / sizeof(dirEntry_t);

typedef struct directoryBlock
{
    dirEntry_t entries[DIRECTORY_ENTRIES_PER_BLOCK];
} directoryBlock_t;

typedef struct checkpointEntry
{
    inode_index_t inodeIndex;
    block_index_t inodeLocation;
} checkpoint_entry_t;

typedef struct checkpointBlock
{
    uint32_t magic;
    uint64_t sequenceNumber;
    uint64_t timestamp;
    uint32_t checkpointID;
    bool isHeader;
    uint32_t numEntries;
    block_index_t nextCheckpointBlock;
    uint8_t reserved[24]; // aligns the metadata part to 64 bytes
    checkpoint_entry_t entries[NUM_CHECKPOINTENTRIES_PER_CHECKPOINT];

} checkpointBlock_t;

typedef union block
{
    uint8_t data[4096];
    inodeBlock_t inodeBlock;
    directBlock_t directBlock;
    logEntry_t logEntry;
    checkpointBlock_t checkpointBlock;
    superBlock_t superBlock;
    bitmapBlock_t bitmapBlock;
    inodeTableBlock_t inodeTable;
    directoryBlock_t directoryBlock;
} block_t;

} // namespace fs
#endif //BLOCK_H


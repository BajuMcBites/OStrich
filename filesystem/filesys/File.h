//
// Created by ameya on 2/26/2025.
//

#ifndef FILE_H
#define FILE_H
#include "BitmapManager.h"
#include "InodeTable.h"
#include "LogManager.h"

namespace fs {
class File
{
public:
    File(InodeTable* inodeTable, BitmapManager* inodeBitmap, BitmapManager* blockBitmap, BlockManager* blockManager, LogManager* logManager,
         uint16_t permissions = 0);
    File(inode_index_t inodeNumber, InodeTable* inodeTable, BitmapManager* inodeBitmap, BitmapManager* blockBitmap,
         BlockManager* blockManager, LogManager* logManager);
    explicit File(const File* file);
    inode_index_t getInodeNumber() const;
    bool isDirectory() const;

    bool write_at(uint64_t offset, const uint8_t* data, uint64_t size);
    bool read_at(uint64_t offset, uint8_t* data, uint64_t size) const;
    uint64_t getSize() const;

    virtual ~File() = default;

protected:
    InodeTable* inodeTable;
    BitmapManager* inodeBitmap;
    BitmapManager* blockBitmap;
    BlockManager* blockManager;
    LogManager* logManager;

    inode_index_t inodeNumber;
    inode_index_t inodeLocation;

    inode_t inode{};


    bool read_block_data(block_index_t blockNum, uint8_t* data) const;
    bool write_block_data(block_index_t blockNum, const uint8_t* data);
    bool write_new_block_data(const uint8_t* data);

private:
    block_index_t getBlockLocation(block_index_t blockNum) const;
};

} // namespace fs
#endif //FILE_H

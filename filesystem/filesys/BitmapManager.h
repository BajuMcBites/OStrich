//
// Created by ameya on 2/20/2025.
//

#ifndef BITMAPMANAGER_H
#define BITMAPMANAGER_H
#include "Block.h"
#include "../interface/BlockManager.h"

#define NULL_INDEX UINT32_MAX

namespace fs {

class BitmapManager
{
public:
    BitmapManager(block_index_t startBlock, block_index_t numBlocks, block_index_t size, BlockManager* blockManager, block_index_t offset = 0);
    block_index_t findNextFree();
    bool setAllocated(block_index_t index);
    bool setUnallocated(block_index_t index);
    block_index_t getStartBlock() const { return startBlock; }

private:
    static constexpr block_index_t NUM_PARTS = BlockManager::BLOCK_SIZE / sizeof(uint64_t);
    void loadBitmap(block_index_t offset);
    bool saveBitmap();

    block_index_t startBlock;
    block_index_t numBlocks;
    block_index_t size;
    BlockManager* blockManager;

    block_t loadedBlock{};
    block_index_t loadedBlockIndex = NULL_INDEX;
    bool dirty = false;
    bool transparentOffset;
    block_index_t additionalOffset;
};

} // namespace fs
#endif //BITMAPMANAGER_H
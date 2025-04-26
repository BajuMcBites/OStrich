//
// Created by ameya on 2/20/2025.
//

#include "BitmapManager.h"
#include "cassert"
#include "cstdio"

namespace fs {

BitmapManager::BitmapManager(const block_index_t startBlock, const block_index_t numBlocks, const block_index_t size,
                             BlockManager* blockManager, block_index_t offset): startBlock(startBlock),
    numBlocks(numBlocks), size(size),
    blockManager(blockManager), additionalOffset(offset)
{
//    cout << "loading bitmap with start block: " << startBlock << " num blocks: " << numBlocks << " size: " << size << endl;
    loadBitmap(0);
}

void BitmapManager::loadBitmap(const block_index_t offset)
{
    if (offset >= numBlocks)
    {
        printf("Offset out of bounds\n");
        assert(0);
    }
    if (dirty)
    {
        saveBitmap();
    }
    if (!blockManager->readBlock(startBlock + offset, loadedBlock.data))
    {
        printf("Could not read bitmap block\n");
        assert(0);
    }
    dirty = false;
    loadedBlockIndex = offset;
}

bool BitmapManager::saveBitmap()
{
    if (loadedBlockIndex == NULL_INDEX)
    {
        printf("Attempted to save invalid block index\n");
        return false;
    }

    if (!blockManager->writeBlock(startBlock + loadedBlockIndex, loadedBlock.data))
    {
        printf("Could not write bitmap block\n");
        assert(0);
    }
    dirty = false;
    return true;
}

block_index_t BitmapManager::findNextFree()
{
    if (loadedBlockIndex == NULL_INDEX)
    {
        loadBitmap(0);
    }
    block_index_t curIteration = 0;
    while (curIteration < numBlocks)
    {
        for (size_t i = 0; i < NUM_PARTS; i++)
        {
            if (loadedBlock.bitmapBlock.parts[i] != UINT64_MAX)
            {
                for (uint8_t j = 0; j < 64; j++)
                {
                    if (!(loadedBlock.bitmapBlock.parts[i] & 1ULL << j))
                    {
                        return additionalOffset + (loadedBlockIndex * BlockManager::BLOCK_SIZE * 8 +
                            i * sizeof(uint64_t) * 8 + j);
                    }
                }
            }
        }
        loadBitmap((loadedBlockIndex + 1) % numBlocks);
        curIteration++;
    }
    return NULL_INDEX;
}

bool BitmapManager::setAllocated(block_index_t index)
{
    index -= additionalOffset;
    if (index >= size)
    {
        printf("Index out of bounds for bitmap\n");
        return false;
        // assert(0);
    }

    const block_index_t targetOffset = index / 8 / BlockManager::BLOCK_SIZE;
    const block_index_t targetPart = (index % (8 * BlockManager::BLOCK_SIZE)) / (sizeof(uint64_t) * 8);
    const uint8_t targetJ = index % (sizeof(uint64_t) * 8);

    if (loadedBlockIndex != targetOffset)
    {
        loadBitmap(targetOffset);
    }

    loadedBlock.bitmapBlock.parts[targetPart] |= 1ULL << targetJ;
    dirty = true;
    return true;
}

bool BitmapManager::setUnallocated(block_index_t index)
{
    index -= additionalOffset;
    if (index >= size)
    {
        printf("Index out of bounds for bitmap\n");
        return false;
        // assert(0);
    }

    const block_index_t targetOffset = index / 8 / BlockManager::BLOCK_SIZE;
    const block_index_t targetPart = (index % (8 * BlockManager::BLOCK_SIZE)) / (sizeof(uint64_t) * 8);
    const uint8_t targetJ = index % (sizeof(uint64_t) * 8);

    if (loadedBlockIndex != targetOffset)
    {
        loadBitmap(targetOffset);
    }

    loadedBlock.bitmapBlock.parts[targetPart] &= ~(1ULL << targetJ);
    dirty = true;
    return true;
}

} // namespace fs

#include "../filesystem/interface/BlockManager.h"
#include "partition.h"
#include "sdio.h"

namespace fs {

bool BlockManager::readBlock(size_t blockIndex, uint8_t* buffer) {
    int result = sd_read_block(blockIndex + this->startSector, (unsigned char*)buffer, 1);
    return result > 0;
}

bool BlockManager::writeBlock(size_t blockIndex, const uint8_t* buffer) {
    int result = sd_write_block((unsigned char*)buffer, blockIndex + this->startSector, 1);
    return result > 0;
}

}  // namespace fs
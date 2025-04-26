#ifndef LOG_MANAGER_H
#define LOG_MANAGER_H

#include "Block.h"
#include "LogRecord.h"
#include "../interface/BlockManager.h"
#include "BitmapManager.h"
// #include "SpinLock.h"     // Will need to use spinlock from kernel team
#include "stdint.h"
#include "cstring"

#include "InodeTable.h"

// Maximum number of checkpoints we will store.
#define MAX_CHECKPOINTS 64
#define RECORD_MAGIC 0x4A4B4C4D
#define ENTRY_MAGIC 0x3A3B3C3D
#define CHECKPOINT_MAGIC 0x5A5B5C5D

namespace fs {

class LogManager {
public:
    // Constructor: pass a pointer to BlockManager and specify the log area (starting block and number of blocks).
    LogManager(BlockManager* blockManager, BitmapManager* blockBitmap, InodeTable* inode_table, uint32_t startBlock, uint32_t numBlocks, uint64_t latestSystemSeq);

    // append to the log
    bool logOperation(LogOpType opType, LogRecordPayload* payload);


    // Recovery: replay log entries from the last checkpoint (simplified).
    bool recover();

    // Create a checkpoint (lock fileystem, read current inode table, create sufficient checkpoint blocks and write
    // to disk, then create return a checkpoint logrecord that points to the new checkpoint). Note that this does not
    // actually write to the log or update the superblock - the expecation is that this will be done by the caller
    bool createCheckpoint();

    // Mount as a read-only snapshot based on a checkpoint ID.
    bool mountReadOnlySnapshot(uint32_t checkpointID);

    uint64_t globalSequence;   // current system log sequence number

private:
    BlockManager* blockManager;
    InodeTable* inodeTable;
    BitmapManager* blockBitmap;
    uint32_t logStartBlock; // starting block of the dedicated log area
    uint32_t logNumBlocks;  // number of blocks allocated for the log area

    bool applyCheckpoint(block_index_t checkpointBlockIndex);


    // // Spinlock to protect log operations.
    // SpinLock logLock;




    // Current log entry
    logEntry_t currentLogEntry;

    // Helper: Get a timestamp (assumed to be provided by the kernel).
    uint64_t get_timestamp();
};

} // namespace fs
#endif // LOG_MANAGER_H
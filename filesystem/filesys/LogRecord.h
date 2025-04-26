//
// Created by caleb on 2/26/2025.
//

#ifndef LOGRECORD_H
#define LOGRECORD_H

#include "cstdint"

namespace fs {

constexpr uint16_t NUM_LOGRECORDS_PER_LOGENTRY = 127;

// Enum for the different log operation types.
enum class LogOpType : uint16_t {
    LOG_OP_NONE = 0,
    LOG_OP_INODE_UPDATE,
    LOG_OP_INODE_ADD,
    LOG_OP_INODE_DELETE,
    LOG_UPDATE_CHECKPOINT,
    // Add more operation types as needed.
};

// Payload for adding an inode to the inode table.
struct InodeAddPayload {
    uint32_t inodeIndex;       // Newly created inode index.
    uint32_t inodeLocation; // Block number of the actual inode
    uint8_t reserved[8];     // Reserved space to fill payload to 16 bytes.
};

// Payload for replacing an existing inode in the inode table.
struct InodeUpdatePayload {
    uint32_t inodeIndex;       // inode index to modify.
    uint32_t inodeLocation; // Block number of the actual inode
    uint8_t reserved[8];     // Reserved space to fill payload to 16 bytes.
};

// Payload for a bitmap operation (set or clear).
struct InodeDeletePayload {
    uint32_t inodeIndex;       // inode index to remove from inode table.
    uint8_t reserved[12];     // Reserved space to fill payload to 16 bytes.
};

// Payload for a directory entry addition operation.
struct CheckpointPayload {
    uint32_t checkpointLocation;       // Block number of the checkpoint header.
    uint8_t reserved[12];     // Reserved space to fill payload to 16 bytes.
};

// Union that combines all possible payload types (16 bytes).
union LogRecordPayload {
    InodeAddPayload inodeAdd;
    InodeUpdatePayload inodeUpdate;
    InodeDeletePayload inodeDelete;
    CheckpointPayload checkpoint;
    // Add additional payload types as needed.
};

// Log record structure (exactly 32 bytes) for a single operation.
typedef struct logRecord {
    uint64_t sequenceNumber;    // Incremental sequence number (8 bytes).
    uint32_t magic;             // Magic for log validation (4 bytes).
    LogOpType opType;           // Operation type (2 bytes)
    uint16_t padding;           // Padding (2 bytes).
    LogRecordPayload payload;   // Operation-specific payload (16 bytes).
} logRecord_t;

// Log entry structure that fits exactly one 4KB block.
typedef struct logEntry {
    uint32_t magic;             // Magic constant for log block validation.
    uint16_t numRecords;        // Number of current log records in this entry (max 127).
    uint8_t reserved[26];      // Reserved to pad the header to 32 bytes.
    logRecord_t records[NUM_LOGRECORDS_PER_LOGENTRY];      // 127 log records of 32 bytes each (127 * 32 = 4064 bytes).
} logEntry_t;



} // namespace fs
#endif //LOGRECORD_H

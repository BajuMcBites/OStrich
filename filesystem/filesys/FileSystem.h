//
// Created by ameya on 2/20/2025.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "BitmapManager.h"
#include "Block.h"
#include "Directory.h"
#include "InodeTable.h"
#include "../interface/BlockManager.h"
#include "LogManager.h"

// Make filesystem a singleton (at most one global instance is allowed to exist).
// Don't call constructor directly, use getInstance instead.
namespace fs {
class FileSystem {
public:
    // Delete copy constructor and assignment operator.
    FileSystem(FileSystem &other) = delete;
    void operator=(const FileSystem &) = delete;

    // Get the singleton instance.
    static FileSystem* getInstance(BlockManager *blockManager = nullptr);

    Directory* getRootDirectory() const;
    bool createCheckpoint();

    // Mount a read-only snapshot based on a checkpoint ID.
    // Returns a new FileSystem instance representing the snapshot, or nullptr on failure.
    FileSystem* mountReadOnlySnapshot(uint32_t checkpointID);

    // make public for now
    InodeTable *inodeTable;
    BitmapManager *inodeBitmap;
    BitmapManager *blockBitmap;
    BlockManager *blockManager;
    LogManager* logManager;

private:
    // Constructor is private, so it can't be called directly.
    explicit FileSystem(BlockManager *blockManager);
    static FileSystem* instance;

    bool readOnly = false; // default false





    block_t superBlockWrapper{};
    superBlock_t* superBlock;

    void createFilesystem();
    void loadFilesystem();
    bool readInode(inode_index_t inodeLocation, inode_t& inode);
    bool writeInode(inode_index_t inodeLocation, inode_t& inode);
    Directory* createRootInode();
};

} // namespace fs
#endif //FILESYSTEM_H

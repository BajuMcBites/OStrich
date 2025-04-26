//
// Created by ameya on 3/11/2025.
//

#ifndef DIRECTORY_H
#define DIRECTORY_H
#include "File.h"
#include "LogManager.h"
#include "vector"

namespace fs {

class Directory : public File
{
public:
    Directory(InodeTable* inodeTable, BitmapManager* inodeBitmap, BitmapManager* blockBitmap,
              BlockManager* blockManager, LogManager* logManager,
              uint16_t permissions = DIRECTORY_MASK);
    Directory(inode_index_t inodeNum, InodeTable* inodeTable, BitmapManager* inodeBitmap, BitmapManager* blockBitmap,
              BlockManager* blockManager, LogManager* logManager);
    inode_index_t getDirectoryEntry(const char* fileName) const;
    bool addDirectoryEntry(const char* fileName, inode_index_t fileNum);
    bool removeDirectoryEntry(const char* fileName);
    bool modifyDirectoryEntry(const char* fileName, inode_index_t fileNum);
    std::vector<char*> listDirectoryEntries() const;
    Directory* createDirectory(const char* name);
    File* createFile(const char* name);
    File* getFile(const char* name) const;

private:
    explicit Directory(const File* file);
    static Directory* convertFile(File* file);
};

} // namespace fs
#endif //DIRECTORY_H
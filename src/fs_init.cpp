#include "fs_init.h"

#include "../filesystem/filesys/FileSystem.h"
#include "../filesystem/interface/BlockManager.h"
#include "heap.h"
#include "libk.h"
#include "partition.h"
#include "printf.h"
#include "sdio.h"
#include "utils.h"

using namespace fs;

/**
 * @brief Get the filesystem partition information.
 *
 * @param fs_start_sector Pointer to store the start sector of the filesystem partition.
 * @param fs_nums_sectors Pointer to store the number of blocks in the filesystem partition.
 */
void get_fs_partition_info(int& fs_start_sector, int& fs_nums_sectors) {
    // Read partition table.
    PartitionEntry partitions[4];
    int ret = read_partition_table(partitions);
    if (ret < 0) {
        printf("Failed to read partition table.\n");
        return;
    }

    // Find the filesystem partition.
    for (int i = 0; i < 4; i++) {
        if (partitions[i].system_id == FS_PARTITION_ID) {
            fs_start_sector = partitions[i].relative_sector;
            fs_nums_sectors = partitions[i].total_sectors;
        }
    }

    K::assert(fs_start_sector != -1 && fs_nums_sectors != -1,
              "Failed to find filesystem partition.");
}

void displayTree(const Directory* dir, const char* curPath) {
    const std::vector<char*> entries = dir->listDirectoryEntries();
    for (const auto entry : entries) {
        File* file = dir->getFile(entry);
        if (file->isDirectory()) {
            printf("%s%s (directory)\n", curPath, entry);
            size_t newPathLen = K::strlen("    ") + K::strlen(curPath) + K::strlen(entry) + 1 +
                                1;  // +1 for '/', +1 for '\0'
            char* newPath = new char[newPathLen];
            K::strcpy(newPath, "    ");
            K::strcat(newPath, curPath);
            K::strcat(newPath, entry);
            K::strcat(newPath, "/");
            displayTree((Directory*)file, newPath);

            delete[] newPath;
        } else {
            if (file->getSize() > 0) {
                int fs = file->getSize();
                if (fs > 256) {
                    fs = 256;
                }

                char* data = new char[fs];
                file->read_at(0, reinterpret_cast<uint8_t*>(data), fs);
                data[fs] = '\0';  // printf capped at 256 characters.
                printf("%s%s (data: %s)\n", curPath, entry, data);
                delete[] data;
            } else {
                printf("%s%s (empty file)\n", curPath, entry);
            }
        }
        delete file;
    }
    for (auto entry : entries) {
        delete[] entry;
    }
}

void displayFilesystem() {
    FileSystem* fileSystem = FileSystem::getInstance();
    printf("Loading root directory...\n");
    auto* rootDir = fileSystem->getRootDirectory();
    displayTree(rootDir, "/");
    FileSystem* snapshotFS = fileSystem->mountReadOnlySnapshot(2);
    if (snapshotFS == nullptr) {
        printf("Failed to mount read-only snapshot.\n");
        return;
    }
    auto* snapRoot = snapshotFS->getRootDirectory();
    printf("\nSnapshot filesystem (checkpoint 2):\n");
    displayTree(snapRoot, "    /");

    printf("Unmounting snapshot filesystem...\n");
    delete snapRoot;
    delete snapshotFS;
    delete rootDir;
}

void fs_init() {
    int fs_start_sector = -1;
    int fs_nums_sectors = -1;
    get_fs_partition_info(fs_start_sector, fs_nums_sectors);
    auto blockManager = new BlockManager(fs_nums_sectors, fs_start_sector);
    printf("Block manager created with %d blocks, starting at sector %d\n", fs_nums_sectors,
           fs_start_sector);
    FileSystem::getInstance(blockManager);

    auto* fileSystem = FileSystem::getInstance();
}

void test_fs() {
    printf("test_fs(): Testing file system on core %d\n", getCoreID());
    auto* fileSystem = FileSystem::getInstance();

    printf("File system ptr: 0x%x\n", fileSystem);
    printf("Inode table ptr: 0x%x\n", fileSystem->inodeTable);
    printf("Inode bitmap ptr: 0x%x\n", fileSystem->inodeBitmap);
    printf("Block bitmap ptr: 0x%x\n", fileSystem->blockBitmap);
    printf("Block manager ptr: 0x%x\n", fileSystem->blockManager);
    printf("Log manager ptr: 0x%x\n", fileSystem->logManager);

    printf("Loading root directory\n");
    auto* rootDir = fileSystem->getRootDirectory();

    printf("Creating new directory /dir1\n");
    Directory* dir1 = rootDir->createDirectory("dir1");

    printf("Creating new file /dir1/file1\n");
    File* file1 = dir1->createFile("file1");

    printf("Writing data to file1\n");
    const char* testData = "testing!";
    file1->write_at(0, (uint8_t*)testData, strlen(testData) + 1);

    printf("\n");

    printf("Creating new directory /dir2\n");
    Directory* dir2 = rootDir->createDirectory("dir2");

    printf("Creating a new checkpoint\n");
    fileSystem->createCheckpoint();

    printf("\n");

    printf("Creating new directory /dir2/dir3\n");
    Directory* dir3 = dir2->createDirectory("dir3");

    printf("Creating new file /dir2/dir3/file2\n");
    File* file2 = dir3->createFile("file2");

    printf("\n");

    printf("Creating new file /dir2/tmpfile\n");
    File* tmpFile = dir2->createFile("tmpfile");

    printf("Creating new file /dir2/largefile\n");
    File* largeFile = dir2->createFile("largefile");

    printf("Writing >BLOCK_SIZE bytes to largefile\n");
    constexpr uint64_t BUFFER_SIZE = BlockManager::BLOCK_SIZE + 100;
    char* buffer = new char[BUFFER_SIZE];
    for (uint64_t i = 0; i < BUFFER_SIZE - 1; i++) {
        buffer[i] = 'a';
    }
    buffer[BUFFER_SIZE - 1] = '\0';
    largeFile->write_at(0, (uint8_t*)buffer, BUFFER_SIZE);

    printf("Creating a new checkpoint\n");
    fileSystem->createCheckpoint();

    printf("Changing data in largefile\n");
    auto newTestData = " - new data! - ";
    largeFile->write_at(10, (uint8_t*)newTestData,
                        strlen(newTestData));  // don't copy null terminator

    printf("Deleting /dir2/tmpfile\n");
    dir2->removeDirectoryEntry("tmpfile");

    displayFilesystem();

    delete[] buffer;
    delete tmpFile;
    delete file1;
    delete file2;
    delete largeFile;
    delete dir1;
    delete dir2;
    delete dir3;
    delete rootDir;

    printf("Filesystem test finished!\n");
}

void testSnapshot() {
    printf("FS SNAPSHOT TEST\n");
    // Create live filesystem instance and display its state.
    FileSystem* liveFS = FileSystem::getInstance();
    Directory* liveRoot = liveFS->getRootDirectory();
    printf("Live filesystem:\n");
    displayTree(liveRoot, "/");
    delete liveRoot;

    // Mount a read-only snapshot based on a checkpoint.
    // (Adjust the checkpointID as needed; here we use 2 as an example.)
    FileSystem* snapshotFS = liveFS->mountReadOnlySnapshot(2);
    if (snapshotFS == nullptr) {
        printf("Failed to mount read-only snapshot.\n");
        return;
    }

    Directory* snapRoot = snapshotFS->getRootDirectory();
    printf("\nRead-only snapshot (checkpoint 2):\n");
    displayTree(snapRoot, "/");
    delete snapRoot;

    FileSystem* snapshotFS2 = liveFS->mountReadOnlySnapshot(3);
    if (snapshotFS2 == nullptr) {
        printf("Failed to mount read-only snapshot.\n");
        return;
    }
    auto* snapRoot2 = snapshotFS2->getRootDirectory();
    printf("\nRead-only snapshot (checkpoint 3):\n");
    displayTree(snapRoot2, "/");
    delete snapRoot2;

    // Optionally, try a write operation on the snapshot to ensure it is read-only.
    // For example:
    // Directory* snapDir = (Directory*)snapshotFS->getRootDirectory()->getFile("dir1");
    // if (snapDir) {
    //     if (!snapDir->createFile("illegalWrite")) {
    //         printf("\nWrite operation correctly rejected in snapshot.\n");
    //     } else {
    //         printf("\nError: Write operation succeeded in snapshot!\n");
    //     }
    //     delete snapDir;
    // }

    // Clean up the snapshot instance.
    delete snapshotFS;
    delete snapshotFS2;
}
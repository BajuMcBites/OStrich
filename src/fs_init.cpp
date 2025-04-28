#include "fs_init.h"

#include "../filesystem/filesys/FileSystem.h"
#include "../filesystem/filesys/fs_requests.h"
#include "../filesystem/interface/BlockManager.h"
#include "heap.h"
#include "libk.h"
#include "partition.h"
#include "printf.h"
#include "sdio.h"
#include "string.h"

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

static string readFile(inode_index_t inode, int nBytes) {
    char buffer[256] = {};
    auto rdr = fs::fs_req_read(inode, buffer, 0, nBytes);
    buffer[nBytes - 1] = '\0';
    K::assert(rdr.status == fs::FS_RESP_SUCCESS, "");
    return string(buffer);
}

static bool dirContains(inode_index_t dirInode, const char* name) {
    auto resp = fs::fs_req_read_dir(dirInode);
    printf("dirContains: %d entries in inode %u\n", resp.entry_count, dirInode);
    K::assert(resp.status == fs::FS_RESP_SUCCESS, "Failed to read directory.");
    for (int i = 0; i < resp.entry_count; i++) {
        printf("entry %d: %s\n", i, resp.entries[i].name);
        if (K::strcmp(resp.entries[i].name, name) == 0) {
            return true;
        }
    }
    return false;
}

// Test stuff down here.
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

                // printf capped at 256 characters.
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

void displayFileTree() {
    auto* fileSystem = FileSystem::getInstance();
    auto* rootDir = fileSystem->getRootDirectory();
    displayTree(rootDir, "/");
}

void test_fs() {
    printf("FS TEST\n");
    auto* fileSystem = FileSystem::getInstance();

    printf("Loading root directory\n");
    auto* rootDir = fileSystem->getRootDirectory();

    printf("Creating new directory /dir1\n");
    Directory* dir1 = rootDir->createDirectory("dir1");

    printf("Creating new file /dir1/file1\n");
    File* file1 = dir1->createFile("file1");

    printf("Writing data to file1\n");
    const char* testData = "testing!";
    file1->write_at(0, (uint8_t*)testData, K::strlen(testData) + 1);

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
                        K::strlen(newTestData));  // don't copy null terminator

    printf("Deleting /dir2/tmpfile\n");
    dir2->removeDirectoryEntry("tmpfile");

    displayTree(rootDir, "/");

    delete[] buffer;
    delete tmpFile;
    delete file1;
    delete file2;
    delete largeFile;
    delete dir1;
    delete dir2;
    delete dir3;
    delete rootDir;

    printf("FS TEST FINISHED!\n");
}

void testSnapshot() {
    printf("FS SNAPSHOT TEST\n");
    // // Create live filesystem instance and display its state.
    FileSystem* liveFS = FileSystem::getInstance();
    Directory* liveRoot = liveFS->getRootDirectory();
    displayTree(liveRoot, "/");
    printf("Live filesystem:\n");
    displayTree(liveRoot, "/");

    // Mount a read-only snapshot based on a checkpoint.
    // (Adjust the checkpointID as needed; here we use 2 as an example.)
    if (!liveFS->mountReadOnlySnapshot(2)) {
        printf("Failed to mount read-only snapshot.\n");
        return;
    }

    Directory* snapRoot = liveFS->getRootDirectory();
    printf("\nRead-only snapshot (checkpoint 2):\n");
    displayTree(snapRoot, "/");
    delete snapRoot;

    if (!liveFS->mountReadOnlySnapshot(3)) {
        printf("Failed to mount read-only snapshot.\n");
        return;
    }
    auto* snapRoot2 = liveFS->getRootDirectory();
    printf("\nRead-only snapshot (checkpoint 3):\n");
    displayTree(snapRoot2, "/");
    delete snapRoot2;

    if (!liveFS->mountReadOnlySnapshot(0)) {
        printf("Failed to mount latest FS state.\n");
        return;
    }

    printf("FS SNAPSHOT TEST FINISHED!\n");
}

void test_fs_requests() {
    auto* fileSystem = FileSystem::getInstance();

    printf("FS REQUESTS TEST\n");
    constexpr int ROOT_DIR_INODE = 0;

    // 1) CREATE file1 in root directory.
    printf("STEP 1\n");
    printf("Creating file1 in root directory...\n");
    auto r1 = fs_req_create_file(ROOT_DIR_INODE, false, "file1", 0);
    K::assert(r1.status == FS_RESP_SUCCESS, "Failed to create file1 in root directory.");
    printf("File1 created successfully.\n");

    // assert it shows up in root
    K::assert(dirContains(ROOT_DIR_INODE, "file1"), "file1 not foundin root dir.");
    printf("File1 found in root directory.\n");

    printf("STEP 2\n");
    // 2) WRITE "hello"
    // open to fetch inode number
    auto r2 = fs_req_open("/file1");
    K::assert(r2.status == FS_RESP_SUCCESS, "Failed to open file1.");
    inode_index_t file1Inode = r2.inode_index;
    printf("File1 opened successfully.\n");

    const char* msg = "hello";
    int len = K::strlen(msg) + 1;  // includes null terminator
    auto r3 = fs_req_write(file1Inode, msg, 0, len);
    K::assert(r3.status == FS_RESP_SUCCESS, "Failed to write to file1.");
    printf("Data written to file1 successfully.\n");

    // assert read returns the same data
    string got = readFile(file1Inode, len);
    K::assert(got == "hello", "contents were incorrect.");
    printf("File1 contents verified successfully.\n");

    printf("STEP 3\n");
    auto r4 = fs_req_remove_file(0, "file1");
    K::assert(r4.status == FS_RESP_SUCCESS, "Failed to remove file1.");

    // assert it's gone
    K::assert(!dirContains(0, "file1"), "file1 still exists.");

    printf("STEP 4\n");
    // recreate
    auto r5 = fs_req_create_file(0, false, "file2", 0);
    K::assert(r5.status == FS_RESP_SUCCESS, "Failed to create file2.");
    file1Inode = r5.inode_index;
    // check that file exists

    // assert it shows up in root
    K::assert(dirContains(ROOT_DIR_INODE, "file2"), "file2 not foundin root dir.");
    printf("File2 found in root directory.\n");

    printf("STEP 5\n");
    auto r6 = fs_req_open("/file2");
    K::assert(r6.status == FS_RESP_SUCCESS, "Failed to open file2.");
    inode_index_t checkInode = r6.inode_index;
    K::assert(checkInode == file1Inode, "file2 inode number mismatch.");

    // write first
    printf("STEP 6\n");
    const char* m1 = "first";
    len = K::strlen(m1) + 1;
    auto r7 = fs_req_write(file1Inode, m1, 0, len);
    K::assert(r7.status == FS_RESP_SUCCESS, "Failed to write to file2.");

    // checkpoint (2)
    K::assert(fileSystem->createCheckpoint(), "Failed to create checkpoint.");

    // overwrite
    printf("STEP 7\n");
    const char* m2 = "second";
    len = K::strlen(m2) + 1;
    auto r8 = fs_req_write(file1Inode, m2, 0, len);
    K::assert(r8.status == FS_RESP_SUCCESS, "Failed to write to file2.");

    // checkpoint (3)
    K::assert(fileSystem->createCheckpoint(), "Failed to create checkpoint.");

    // delete
    printf("STEP 8\n");
    auto r9 = fs_req_remove_file(0, "file2");
    K::assert(r9.status == FS_RESP_SUCCESS, "Failed to remove file2.");

    // checkpoint (4)
    K::assert(fileSystem->createCheckpoint(), "Failed to create checkpoint.");

    // helper to test
    auto validate = [&](int cp, bool exists, const char* expected) {
        printf("----- VALIDATING SNAPSHOT %d -----\n", cp);
        auto resp = fs_req_mount_snapshot(cp);
        K::assert(resp.status == FS_RESP_SUCCESS, "Failed to mount snapshot.");
        printf("snapshot mounted successfully.\n");
        bool found = dirContains(0, "file2");
        K::assert(found == exists, "file2 not found in snapshot.");
        if (exists) {
            int expectedLen = K::strlen(expected) + 1;
            string content = readFile(file1Inode, expectedLen);
            K::assert(content == expected, "contents were incorrect.");
        }
    };

    // TODO: fix this once snapshotting is fixed.
    // validate(2, true, "first");
    // validate(3, true, "second");
    // validate(4, false, nullptr);
    printf("Mounting latest FS state...\n");
    auto resp = fs_req_mount_snapshot(0);
    K::assert(resp.status == FS_RESP_SUCCESS, "Failed to mount snapshot.");

    {
        // Create file named "test1.txt" in root directory.
        printf("Creating test1.txt in root directory...\n");
        auto r1 = fs_req_create_file(0, false, "test1.txt", 0);
        K::assert(r1.status == FS_RESP_SUCCESS, "Failed to create test1.txt in root directory.");
        printf("test1.txt created successfully.\n");

        inode_index_t fileInode = r1.inode_index;
        printf("fileInode: %d\n", fileInode);
        // Write "Hello, World!" to the file.
        const char* msg = "Hello, World!";
        int len = K::strlen(msg) + 1;  // includes null terminator
        auto r3 = fs_req_write(fileInode, msg, 0, len);
        K::assert(r3.status == FS_RESP_SUCCESS, "Failed to write to test1.txt.");
        printf("Data written to test1.txt successfully.\n");

        // Read the data back from the file.
        char buffer[256];
        auto r4 = fs_req_read(fileInode, buffer, 0, len);
        K::assert(r4.status == FS_RESP_SUCCESS, "Failed to read from test1.txt.");
        string readData = string(buffer);
        printf("Read data: %s\n", readData.c_str());
        K::assert(readData == msg, "Data read from test1.txt is incorrect.");
        printf("Data read from test1.txt successfully.\n");

        // Modify the file.
        const char* newMsg = "Hello, Universe!";
        int newLen = K::strlen(newMsg) + 1;
        auto r5 = fs_req_write(fileInode, newMsg, 0, newLen);
        K::assert(r5.status == FS_RESP_SUCCESS, "Failed to modify test1.txt.");
        printf("test1.txt modified successfully.\n");

        // Open the file again.
        auto r6 = fs_req_open("/test1.txt");
        K::assert(r6.status == FS_RESP_SUCCESS, "Failed to open test1.txt.");
        inode_index_t checkInode = r6.inode_index;
        if (checkInode != fileInode) {
            printf("inode number mismatch.\n");
        }

        // Read the modified data back.
        auto r7 = fs_req_read(checkInode, buffer, 0, newLen);
        K::assert(r7.status == FS_RESP_SUCCESS, "Failed to read from test1.txt.");
        string readData2 = string(buffer);
        printf("Read data: %s\n", readData2.c_str());
        K::assert(readData2 == newMsg, "Data read from test1.txt is incorrect.");
        printf("Data read from test1.txt successfully.\n");
    }
}
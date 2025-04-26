#include <cstring>

#include "interface/BlockManager.h"
#include "interface/FakeDiskDriver.h"
#include "filesys/Block.h"
#include "filesys/FileSystem.h"
using namespace fs;
void runFilesystemSetupTest(BlockManager& blockManager)
{
    FileSystem* fileSystem = FileSystem::getInstance(&blockManager);
    std::cout << "Filesystem pointer address is " << fileSystem << std::endl;
    std::cout << "Loading root directory" << std::endl;
    auto* rootDir = fileSystem->getRootDirectory();

    std::cout << "Creating new directory /dir1" << std::endl;
    Directory* dir1 = rootDir->createDirectory("dir1");

    std::cout << "Creating new file /dir1/file1" << std::endl;
    File* file1 = dir1->createFile("file1");

    std::cout << "Writing data to file1" << std::endl;
    const char* testData = "testing!";
    file1->write_at(0, (uint8_t*)testData, strlen(testData) + 1);

    std::cout << "Creating new directory /dir2" << std::endl;
    Directory* dir2 = rootDir->createDirectory("dir2");

    std::cout << "Creating a new checkpoint" << std::endl;
    fileSystem->createCheckpoint();

    std::cout << "Creating new directory /dir2/dir3" << std::endl;
    Directory* dir3 = dir2->createDirectory("dir3");

    std::cout << "Creating new file /dir2/dir3/file2" << std::endl;
    File* file2 = dir3->createFile("file2");

    std::cout << "Creating new file /dir2/tmpfile" << std::endl;
    File* tmpFile = dir2->createFile("tmpfile");

    std::cout << "Creating new file /dir2/largefile" << std::endl;
    File* largeFile = dir2->createFile("largefile");

    std::cout << "Writing >BLOCK_SIZE bytes to largefile" << std::endl;
    constexpr uint64_t BUFFER_SIZE = BlockManager::BLOCK_SIZE + 100;
    char* buffer = new char[BUFFER_SIZE];
    for (uint64_t i = 0; i < BUFFER_SIZE - 1; i++)
    {
        buffer[i] = 'a';
    }
    buffer[BUFFER_SIZE - 1] = '\0';
    largeFile->write_at(0, (uint8_t*)buffer, BUFFER_SIZE);
    delete[] buffer;

    std::cout << "Creating a new checkpoint" << std::endl;
    fileSystem->createCheckpoint();

    std::cout << "Changing data in largefile" << std::endl;
    auto newTestData = " - new data! - ";
    largeFile->write_at(10, (uint8_t*)newTestData, strlen(newTestData)); // don't copy null terminator

    std::cout << "Deleting /dir2/tmpfile" << std::endl;
    dir2->removeDirectoryEntry("tmpfile");
    delete tmpFile;



    delete file1;
    delete file2;
    delete largeFile;
    delete dir1;
    delete dir2;
    delete dir3;
    delete rootDir;

    // kill singleton instance
    //delete FileSystem::getInstance();
}

void displayTree(const Directory* dir, const std::string& curPath)
{
    const std::vector<char*> entries = dir->listDirectoryEntries();
    for (const auto entry : entries)
    {
        File* file = dir->getFile(entry);
        if (file->isDirectory())
        {
            std::cout << curPath << entry << " (directory)" << std::endl;
            displayTree(dynamic_cast<Directory*>(file), "    " + curPath + entry + "/");
        }
        else
        {
            if (file->getSize() > 0)
            {
                char* data = new char[file->getSize()];
                file->read_at(0, reinterpret_cast<uint8_t*>(data), file->getSize());
                data[file->getSize() - 1] = '\0';
                std::cout << curPath << entry << " (file: " << data << ")" << std::endl;
                delete[] data;
            }
            else
            {
                std::cout << curPath << entry << " (empty file)" << std::endl;
            }
        }
        delete file;
    }
    for (auto entry : entries)
    {
        delete[] entry;
    }
}

void displayFilesystem(BlockManager& blockManager)
{
    FileSystem* fileSystem = FileSystem::getInstance(&blockManager);
    std::cout << "Loading root directory..." << std::endl;
    auto* rootDir = fileSystem->getRootDirectory();
    std::cout << "/" << std::endl;
    displayTree(rootDir, "    /");
    FileSystem* snapshotFS = fileSystem->mountReadOnlySnapshot(2);
    if (snapshotFS == nullptr) {
        std::cerr << "Failed to mount read-only snapshot." << std::endl;
        return;
    }
    auto* snapRoot = snapshotFS->getRootDirectory();
    std::cout << "\nSnapshot filesystem (checkpoint 2):" << std::endl;
    displayTree(snapRoot, "    /");

    cout << "Unmounting snapshot filesystem..." << endl;
    delete snapRoot;
    delete snapshotFS;
    delete rootDir;
}

// New function to display both the live FS and the snapshot.
void testSnapshot(BlockManager& blockManager)
{
    // Create live filesystem instance and display its state.
    FileSystem *liveFS = FileSystem::getInstance(&blockManager);
    std::cout << "FS pointer address is " << liveFS << std::endl;
    Directory* liveRoot = liveFS->getRootDirectory();
    std::cout << "Live filesystem:" << std::endl;
    displayTree(liveRoot, "    /");
    delete liveRoot;

    // Mount a read-only snapshot based on a checkpoint.
    // (Adjust the checkpointID as needed; here we use 2 as an example.)
    FileSystem* snapshotFS = liveFS->mountReadOnlySnapshot(2);
    if (snapshotFS == nullptr) {
        std::cerr << "Failed to mount read-only snapshot." << std::endl;
        return;
    }
    Directory* snapRoot = snapshotFS->getRootDirectory();
    std::cout << "\nRead-only snapshot (checkpoint 2):" << std::endl;
    displayTree(snapRoot, "    /");
    delete snapRoot;

    FileSystem* snapshotFS2 = liveFS->mountReadOnlySnapshot(3);
    if (snapshotFS2 == nullptr) {
        std::cerr << "Failed to mount read-only snapshot." << std::endl;
        return;
    }
    auto* snapRoot2 = snapshotFS2->getRootDirectory();
    std::cout << "\nRead0only snapshot (checkpoint 3):" << std::endl;
    displayTree(snapRoot2, "    /");
    delete snapRoot2;

    // Optionally, try a write operation on the snapshot to ensure it is read-only.
    // For example:
    // Directory* snapDir = dynamic_cast<Directory*>(snapshotFS->getRootDirectory()->getFile("dir1"));
    // if (snapDir) {
    //     if (!snapDir->createFile("illegalWrite")) {
    //         std::cout << "\nWrite operation correctly rejected in snapshot." << std::endl;
    //     } else {
    //         std::cerr << "\nError: Write operation succeeded in snapshot!" << std::endl;
    //     }
    //     delete snapDir;
    // }

    // Clean up the snapshot instance.
    delete snapshotFS;
    delete snapshotFS2;
}

int main()
{
    FakeDiskDriver disk("OStrich_Hard_Drive", 8192);
    if (!disk.createPartition(0, 8192, "ext4"))
    {
        std::cerr << "Error: Failed to create partition" << std::endl;
        return 1;
    }
    BlockManager block_manager(disk, disk.listPartitions()[0], 1024);
    block_t emptyBlock{};
    block_manager.writeBlock(0, emptyBlock.data); // write empty superblock to force creation of new fs

    std::cout << "Running filesystem setup test" << std::endl;
    runFilesystemSetupTest(block_manager);

    std::cout << "\n\nDisplaying live filesystem and snapshot:" << std::endl;
    testSnapshot(block_manager);

    return 0;
}

#include "FakeDiskDriver.h"
#include <iostream>
#include <thread>

using namespace std;

namespace fs {

// Constructor: Opens (or creates) the file and ensures its size.
FakeDiskDriver::FakeDiskDriver(const string& filename, size_t numSectors,
                               chrono::milliseconds simulatedLatency)
    : diskFilename(filename), totalSectors(numSectors), ioLatency(simulatedLatency)
{
    if (!openDisk())
    {
        cerr << "Error: Could not open disk file " << diskFilename << "\n";
    }
    else if (!ensureDiskSize())
    {
        cerr << "Error: Could not ensure disk file size for " << diskFilename << "\n";
    }
}

// Destructor: Flushes and closes the file.
FakeDiskDriver::~FakeDiskDriver()
{
    flush();
    closeDisk();
}

// openDisk: Opens the file in binary read/write mode, creating it if necessary.
bool FakeDiskDriver::openDisk()
{
    lock_guard<mutex> lock(diskMutex);
    diskFile.open(diskFilename, ios::in | ios::out | ios::binary);
    if (!diskFile.is_open())
    {
        // File does not exist; create it.
        diskFile.clear();
        diskFile.open(diskFilename, ios::out | ios::binary);
        if (!diskFile.is_open())
        {
            cerr << "Error: Failed to create disk file " << diskFilename << "\n";
            return false;
        }
        diskFile.close();
        // Reopen in read/write mode.
        diskFile.open(diskFilename, ios::in | ios::out | ios::binary);
        if (!diskFile.is_open())
        {
            cerr << "Error: Failed to reopen disk file " << diskFilename << "\n";
            return false;
        }
    }
    return true;
}

// closeDisk: Closes the file.
void FakeDiskDriver::closeDisk()
{
    lock_guard<mutex> lock(diskMutex);
    if (diskFile.is_open())
    {
        diskFile.close();
    }
}

// ensureDiskSize: Makes sure the file is large enough to hold all sectors.
bool FakeDiskDriver::ensureDiskSize()
{
    lock_guard<mutex> lock(diskMutex);
    diskFile.clear();
    diskFile.seekg(0, ios::end);
    streampos currentSize = diskFile.tellg();
    size_t requiredSize = totalSectors * SECTOR_SIZE;
    if (static_cast<size_t>(currentSize) < requiredSize)
    {
        // Expand the file to the required size.
        diskFile.clear();
        diskFile.seekp(requiredSize - 1, ios::beg);
        char zero = 0;
        if (!diskFile.write(&zero, 1))
        {
            cerr << "Error: ensureDiskSize: failed to expand disk file\n";
            return false;
        }
        if (!diskFile.flush())
        {
            cerr << "Error: ensureDiskSize: failed to flush disk file\n";
            return false;
        }
    }
    return true;
}

// readSector: Reads a sector from the simulated disk.
bool FakeDiskDriver::readSector(size_t sectorIndex, uint8_t* buffer)
{
    if (sectorIndex >= totalSectors)
    {
        cerr << "Error: readSector: sector index " << sectorIndex << " out of range\n";
        return false;
    }
    lock_guard<mutex> lock(diskMutex);

    // Simulate I/O latency (e.g., SD card delay on the Pi 3).
    this_thread::sleep_for(ioLatency);

    // sector.resize(SECTOR_SIZE);
    diskFile.clear();
    diskFile.seekg(sectorIndex * SECTOR_SIZE, ios::beg);
    if (!diskFile.read(reinterpret_cast<char*>(buffer), SECTOR_SIZE))
    {
        cerr << "Error: readSector: failed to read sector " << sectorIndex << "\n";
        return false;
    }
    return true;
}

// writeSector: Writes a sector to the simulated disk.
bool FakeDiskDriver::writeSector(const size_t sectorIndex, const uint8_t* buffer)
{
    if (sectorIndex >= totalSectors)
    {
        cerr << "Error: writeSector: sector index " << sectorIndex << " out of range\n";
        return false;
    }
    // if (sector.size() != SECTOR_SIZE) {
    //     cerr << "Error: writeSector: sector size mismatch (expected " << SECTOR_SIZE << " bytes)\n";
    //     return false;
    // }

    lock_guard<mutex> lock(diskMutex);

    // Simulate I/O latency (e.g., SD card delay on the Pi 3).
    this_thread::sleep_for(ioLatency);

    diskFile.clear();
    diskFile.seekp(sectorIndex * SECTOR_SIZE, ios::beg);
    if (!diskFile.write(reinterpret_cast<const char*>(buffer), SECTOR_SIZE))
    {
        cerr << "Error: writeSector: failed to write sector " << sectorIndex << "\n";
        return false;
    }
    // For performance, we don’t flush after every sector (unless required).
    return true;
}

// flush: Flushes the file buffers.
bool FakeDiskDriver::flush()
{
    lock_guard<mutex> lock(diskMutex);
    if (diskFile.is_open())
    {
        // Simulate a flush latency.
        this_thread::sleep_for(ioLatency);
        diskFile.flush();
        return true;
    }
    return false;
}

// createPartition: Creates a partition if the sector range is valid and non–overlapping.
bool FakeDiskDriver::createPartition(size_t startSector, size_t sectorCount, const string& type)
{
    if (startSector + sectorCount > totalSectors)
    {
        cerr << "Error: createPartition: partition exceeds disk size\n";
        return false;
    }
    lock_guard<mutex> lock(partitionMutex);
    // Check for overlaps.
    for (const auto& p : partitions)
    {
        size_t pEnd = p.startSector + p.sectorCount;
        size_t newEnd = startSector + sectorCount;
        if (!(newEnd <= p.startSector || startSector >= pEnd))
        {
            cerr << "Error: createPartition: partition overlaps an existing partition\n";
            return false;
        }
    }
    partitions.push_back({startSector, sectorCount, type});
    return true;
}

// deletePartition: Deletes the partition specified by its index.
bool FakeDiskDriver::deletePartition(size_t partitionIndex)
{
    lock_guard<mutex> lock(partitionMutex);
    if (partitionIndex >= partitions.size())
    {
        cerr << "Error: deletePartition: partition index out of range\n";
        return false;
    }
    partitions.erase(partitions.begin() + partitionIndex);
    return true;
}

// listPartitions: Returns the current list of partitions.
vector<FakeDiskDriver::Partition> FakeDiskDriver::listPartitions() const
{
    lock_guard<mutex> lock(partitionMutex);
    return partitions;
}

} // namespace fs

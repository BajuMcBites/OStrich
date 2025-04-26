#ifndef FAKE_DISK_DRIVER_H
#define FAKE_DISK_DRIVER_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

using namespace std;

namespace fs {

class FakeDiskDriver
{
public:
    // A sector is fixed at 512 bytes.
    static constexpr size_t SECTOR_SIZE = 512;
    // using Sector = uint8_t[SECTOR_SIZE];

    // Partition structure for simulation.
    struct Partition
    {
        size_t startSector; // Starting sector index.
        size_t sectorCount; // Number of sectors in the partition.
        std::string type; // Label (for example, "ext4", "FAT32", etc.)
    };

    /**
     * Constructor.
     *
     * @param diskFilename     The filename to use for the simulated disk.
     * @param numSectors        Total number of sectors (disk size = numSectors * SECTOR_SIZE).
     * @param simulatedLatency The artificial latency to simulate disk I/O delays (default: 10ms).
     */
    FakeDiskDriver(const std::string& diskFilename, size_t numSectors,
                   std::chrono::milliseconds simulatedLatency = std::chrono::milliseconds(10));

    // Destructor.
    ~FakeDiskDriver();

    // Disable copy/assignment.
    FakeDiskDriver(const FakeDiskDriver&) = delete;
    FakeDiskDriver& operator=(const FakeDiskDriver&) = delete;

    /**
     * Reads the sector at the given index.
     *
     * @param sectorIndex  The sector number to read.
     * @param buffer       The buffer that the sector data will be written to. Must be at least SECTOR_SIZE bytes.
     * @return true if successful, false otherwise.
     */
    bool readSector(size_t sectorIndex, uint8_t* buffer);

    /**
     * Writes the given sector data at the given sector index.
     *
     * @param sectorIndex  The sector number to write.
     * @param buffer       The buffer containing the data to write. Must be at least SECTOR_SIZE bytes.
     * @return true if successful, false otherwise.
     */
    bool writeSector(size_t sectorIndex, const uint8_t* buffer);

    /**
     * Flushes any pending I/O operations.
     *
     * @return true on success.
     */
    bool flush();

    /**
     * Creates a partition if the specified sector range is valid and does not overlap any existing partition.
     *
     * @param startSector  The starting sector index for the partition.
     * @param sectorCount  The number of sectors for the partition.
     * @param type        A label for the partition.
     * @return true if the partition is created successfully.
     */
    bool createPartition(size_t startSector, size_t sectorCount, const std::string& type);

    /**
     * Deletes a partition given its index.
     *
     * @param partitionIndex  The index of the partition to delete.
     * @return true on success.
     */
    bool deletePartition(size_t partitionIndex);

    /**
     * Returns a copy of the current partition list.
     *
     * @return A vector containing all partitions.
     */
    vector<Partition> listPartitions() const;

private:
    string diskFilename;
    size_t totalSectors;
    fstream diskFile;

    // In–memory partition table.
    vector<Partition> partitions;

    // Mutex to protect disk I/O (the file stream isn’t thread–safe).
    mutable mutex diskMutex;
    // Mutex to protect partition table modifications.
    mutable mutex partitionMutex;

    // Artificial I/O latency to simulate the SD card delay on Raspberry Pi 3.
    chrono::milliseconds ioLatency;

    // Private helper functions.
    bool openDisk();
    void closeDisk();
    bool ensureDiskSize();
};



} // namespace fs

#endif // FAKE_DISK_DRIVER_H

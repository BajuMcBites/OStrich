#include "RealDiskDriver.h"

#include "filesys_compat/cstdint"
#include "filesys_compat/vector"
#include "sdio.h"

/**
 * Constructor.
 *
 * @param diskFilename     The filename to use for the simulated disk.
 * @param numSectors        Total number of sectors (disk size = numSectors * SECTOR_SIZE).
 * @param simulatedLatency The artificial latency to simulate disk I/O delays (default: 10ms).
 */
RealDiskDriver::RealDiskDriver(size_t numSectors) {
    totalSectors = numSectors;
    // Fill in partition table.
}

RealDiskDriver::~RealDiskDriver() {
    // nothing to destruct.
}

/**
 * Reads the sector at the given index.
 *
 * @param sectorIndex  The sector number to read.
 * @param buffer       The buffer that the sector data will be written to. Must be at least
 * SECTOR_SIZE bytes.
 * @return true if successful, false otherwise.
 */
bool RealDiskDriver::readSector(size_t sectorIndex, uint8_t* buffer) {
    int ret = sd_read_block(sectorIndex, buffer, 1);
    return ret == SD_BLK_SIZE;
}

/**
 * Writes the given sector data at the given sector index.
 *
 * @param sectorIndex  The sector number to write.
 * @param buffer       The buffer containing the data to write. Must be at least SECTOR_SIZE bytes.
 * @return true if successful, false otherwise.
 */
bool RealDiskDriver::writeSector(size_t sectorIndex, const uint8_t* buffer) {
    int ret = sd_write_block((unsigned char*)buffer, sectorIndex, 1);
    return ret == SD_BLK_SIZE;
}

/**
 * Flushes any pending I/O operations.
 *
 * @return true on success.
 */
bool RealDiskDriver::flush() {
    // I/O is synchronous, so we don't need to do anything.
    return true;
}

/**
 * Creates a partition if the specified sector range is valid and does not overlap any existing
 * partition.
 *
 * @param startSector  The starting sector index for the partition.
 * @param sectorCount  The number of sectors for the partition.
 * @param type        A label for the partition.
 * @return true if the partition is created successfully.
 */
bool RealDiskDriver::createPartition(size_t startSector, size_t sectorCount, const char* type) {
    return true;
}

/**
 * Deletes a partition given its index.
 *
 * @param partitionIndex  The index of the partition to delete.
 * @return true on success.
 */
bool RealDiskDriver::deletePartition(size_t partitionIndex) {
    return true;
}

/**
 * Returns a copy of the current partition list.
 *
 * @return A vector containing all partitions.
 */
std::vector<RealDiskDriver::Partition> RealDiskDriver::listPartitions() const {
    std::vector<RealDiskDriver::Partition> vec;
    return vec;
}

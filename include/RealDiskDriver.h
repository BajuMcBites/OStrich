#ifndef REAL_DISK_DRIVER_H
#define REAL_DISK_DRIVER_H

#include "filesys_compat/cstdint"
#include "filesys_compat/cstring"
#include "filesys_compat/vector"

using namespace std;

class RealDiskDriver {
   public:
    // A sector is fixed at 512 bytes.
    static constexpr size_t SECTOR_SIZE = 512;
    // using Sector = uint8_t[SECTOR_SIZE];

    // Partition structure for simulation.
    struct Partition {
        size_t startSector;  // Starting sector index.
        size_t sectorCount;  // Number of sectors in the partition.
        const char* type;    // Label (for example, "ext4", "FAT32", etc.)
    };

    /**
     * Constructor.
     *
     * @param numSectors        Total number of sectors (disk size = numSectors * SECTOR_SIZE).
     */
    RealDiskDriver(size_t numSectors);

    // Destructor.
    ~RealDiskDriver();

    // Disable copy/assignment.
    RealDiskDriver(const RealDiskDriver&) = delete;
    RealDiskDriver& operator=(const RealDiskDriver&) = delete;

    /**
     * Reads the sector at the given index.
     *
     * @param sectorIndex  The sector number to read.
     * @param buffer       The buffer that the sector data will be written to. Must be at least
     * SECTOR_SIZE bytes.
     * @return true if successful, false otherwise.
     */
    bool readSector(size_t sectorIndex, uint8_t* buffer);

    /**
     * Writes the given sector data at the given sector index.
     *
     * @param sectorIndex  The sector number to write.
     * @param buffer       The buffer containing the data to write. Must be at least SECTOR_SIZE
     * bytes.
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
     * Creates a partition if the specified sector range is valid and does not overlap any existing
     * partition.
     *
     * @param startSector  The starting sector index for the partition.
     * @param sectorCount  The number of sectors for the partition.
     * @param type        A label for the partition.
     * @return true if the partition is created successfully.
     */
    bool createPartition(size_t startSector, size_t sectorCount, const char* type);

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
    std::vector<Partition> listPartitions() const;

   private:
    size_t totalSectors;

    // In–memory partition table.
    std::vector<Partition> partitions;

    // Private helper functions.
    bool openDisk();
    void closeDisk();
    bool ensureDiskSize();
};

#endif  // REAL_DISK_DRIVER_H

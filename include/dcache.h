#ifndef _DCACHE_H
#define _DCACHE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Clean a memory range in the data cache (write back to memory)
 *
 * Ensures that the specified range of memory is written back from cache to main memory,
 * making it visible to other cores and devices. The data remains valid in the cache.
 *
 * @param start Starting virtual address (should be 64-byte aligned for best performance)
 * @param size Size of the memory range in bytes
 */
void clean_dcache_range(void* start, unsigned long size);

/**
 * @brief Invalidate a memory range in the data cache
 *
 * Marks the specified range of memory as invalid in the cache, forcing future
 * reads to fetch data from main memory.
 *
 * @param start Starting virtual address (should be 64-byte aligned for best performance)
 * @param size Size of the memory range in bytes
 */
void invalidate_dcache_range(void* start, unsigned long size);

/**
 * @brief Clean and invalidate a memory range in the data cache
 *
 * Writes back any dirty cache lines to memory and then invalidates them.
 * This ensures both that changes are visible to other cores/devices and that
 * subsequent reads will fetch the latest data from memory.
 *
 * @param start Starting virtual address (should be 64-byte aligned for best performance)
 * @param size Size of the memory range in bytes
 */
void clean_and_invalidate_dcache_range(void* start, unsigned long size);

/**
 * @brief Clean a single cache line containing the specified address
 *
 * @param addr Virtual address within the cache line to clean
 */
void clean_dcache_line(void* addr);

/**
 * @brief Invalidate a single cache line containing the specified address
 *
 * @param addr Virtual address within the cache line to invalidate
 */
void invalidate_dcache_line(void* addr);

/**
 * @brief Clean and invalidate a single cache line containing the specified address
 *
 * @param addr Virtual address within the cache line to clean and invalidate
 */
void clean_and_invalidate_dcache_line(void* addr);

/**
 * @brief Data memory barrier
 *
 * Ensures that all explicit memory accesses before the barrier
 * complete before any explicit memory accesses after the barrier.
 */
void dmb(void);

/**
 * @brief Data synchronization barrier
 *
 * Ensures that all explicit memory accesses (and cache maintenance operations)
 * before the barrier complete before any explicit memory accesses after the barrier.
 */
void dsb(void);

#ifdef __cplusplus
}
#endif

#endif /* _DCACHE_H */

#ifndef _ICACHE_H
#define _ICACHE_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Invalidate a memory range in the instruction cache.
 *
 * Ensures that any modifications to code in memory are visible to the CPU.
 *
 * @param start Starting virtual address (should be aligned to the cache line size, typically 64
 * bytes)
 * @param size  Size of the memory range in bytes.
 */
void invalidate_icache_range(void* start, unsigned long size);

/**
 * @brief Invalidate the entire instruction cache.
 *
 * Flushes the entire instruction cache so that subsequent instruction fetches
 * retrieve the latest version from memory.
 */
void invalidate_icache_all(void);

/**
 * @brief Instruction Synchronization Barrier.
 *
 * Ensures that any changes to instructions in memory are visible to the CPU.
 */
void isb(void);

#ifdef __cplusplus
}
#endif

#endif /* _ICACHE_H */

#ifndef _VM_H
#define _VM_H

#include "peripherals/base.h"

#define VA_START 0xffff000000000000

#define PAGE_SHIFT 12
#define TABLE_SHIFT 9
#define SECTION_SHIFT (PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE (1 << PAGE_SHIFT)
#define SECTION_SIZE (1 << SECTION_SHIFT)

#define LOW_MEMORY (2 * SECTION_SIZE)
#define HIGH_MEMORY PBASE
#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES (PAGING_MEMORY / PAGE_SIZE)

#define TABLE_ENTRIES (1 << TABLE_SHIFT)

#define VALID_DESCRIPTOR 0x1
#define TABLE_ENTRY 0x2
#define PAGE_ENTRY 0x2

#define READ_PERM 0x1
#define WRITE_PERM 0x2
#define EXEC_PERM 0x4

#define TCR_VALUE                                                                                  \
    ((0b0LL << 40) | /* [40]    HD: HW Dirty Bit Management - 0 = Disabled (0 = SW-managed, 1 =    \
                        HW-managed) */                                                             \
     (0b0LL                                                                                        \
      << 39) | /* [39]    HA: HW Access Flag - 0 = Disabled (0 = SW-managed, 1 = HW-managed) */    \
     (0b0LL << 38) | /* [38]    TBI1: Top Byte Ignore (TTBR1) - 0 = Disabled (0 = Disabled, 1 =    \
                        Enabled) */                                                                \
     (0b0LL << 37) | /* [37]    TBI0: Top Byte Ignore (TTBR0) - 0 = Disabled (0 = Disabled, 1 =    \
                        Enabled) */                                                                \
     (0b1LL << 36) | /* [36]    AS: Address Space ID - 1 = 16-bit ASID (0 = 8-bit, 1 = 16-bit) */  \
     (0b0LL << 35) | /* [35]    Reserved - Must be 0 */                                            \
     (0b000LL << 32) | /* [34:32] IPS: PA Size - 000 = 4GB (000 = 4GB, 001 = 64GB, 010 = 1TB) */   \
     (0b10LL                                                                                       \
      << 30) | /* [31:30] TG1: Granule (TTBR1) - 10 = 4KB (00 = 16KB, 01 = 64KB, 10 = 4KB) */      \
     (0b10LL << 28) | /* [29:28] SH1: Shareability (TTBR1) - 10 = Outer Shareable (00 =            \
                         Non-shareable, 10 = Outer, 11 = Inner) */                                 \
     (0b00LL << 26) | /* [27:26] ORGN1: Outer Cache (TTBR1) - 00 = Non-cacheable (00 = NC, 01 =    \
                         WB, 10 = WT, 11 = WB Non-NC) */                                           \
     (0b00LL << 24) | /* [25:24] IRGN1: Inner Cache (TTBR1) - 00 = Non-cacheable (Same options as  \
                         ORGN1) */                                                                 \
     (0b0LL << 23) |  /* [23]    EPD1: TT Walk (TTBR1) - 0 = Enabled (0 = Walk, 1 = Fault) */      \
     (0b0LL << 22) | /* [22]    A1: ASID Selection - 0 = Uses TTBR0 ASID (0 = TTBR0, 1 = TTBR1) */ \
     (16LL                                                                                         \
      << 16) | /* [21:16] T1SZ: VA Size (TTBR1) - 16 = 256TB (16 = 256TB, 25 = 512GB, 32 = 4GB) */ \
     (0b0LL << 15) | /* [15]    Reserved - Must be 0 */                                            \
     (0b00LL                                                                                       \
      << 14) | /* [15:14] TG0: Granule (TTBR0) - 00 = 4KB (00 = 4KB, 01 = 64KB, 10 = 16KB) */      \
     (0b10LL << 12) | /* [13:12] SH0: Shareability (TTBR0) - 10 = Outer Shareable (00 =            \
                         Non-shareable, 10 = Outer, 11 = Inner) */                                 \
     (0b00LL << 10) | /* [11:10] ORGN0: Outer Cache (TTBR0) - 00 = Non-cacheable (00 = NC, 01 =    \
                         WB, 10 = WT, 11 = WB Non-NC) */                                           \
     (0b00LL << 8) |  /* [9:8]   IRGN0: Inner Cache (TTBR0) - 00 = Non-cacheable (Same options as  \
                         ORGN0) */                                                                 \
     (0b0LL << 7) |   /* [7]     EPD0: TT Walk (TTBR0) - 0 = Enabled (0 = Walk, 1 = Fault) */      \
     (0b0LL << 6) |   /* [6]     Reserved - Must be 0 */                                           \
     (16LL << 0)      /* [5:0]   T0SZ: VA Size (TTBR0) - 16 = 256TB (Same options as T1SZ) */      \
    )
// Memory attribute indices
#define MT_DEVICE_nGnRnE 0  // Device memory: non-Gathering, non-Reordering, non-Early write ack
#define MT_DEVICE_nGnRE 1   // Device memory: non-Gathering, non-Reordering, Early write ack
#define MT_DEVICE_GRE 2     // Device memory: Gathering, Reordering, Early write ack
#define MT_NORMAL_NC 3      // Normal memory: Non-cacheable
#define MT_NORMAL 4         // Normal memory: Cacheable

// Memory Attribute Indirection Register (MAIR) settings
#define MAIR_VALUE                                                                \
    ((0x00ul << (MT_DEVICE_nGnRnE * 8)) | /* [0] Device-nGnRnE */                 \
     (0x04ul << (MT_DEVICE_nGnRE * 8)) |  /* [1] Device-nGnRE */                  \
     (0x0Cul << (MT_DEVICE_GRE * 8)) |    /* [2] Device-GRE */                    \
     (0x44ul << (MT_NORMAL_NC * 8)) |     /* [3] Normal Non-cacheable */          \
     (0xFFul << (MT_NORMAL * 8))          /* [4] Normal memory with write-back */ \
    )

#define DEVICE_LOWER_ATTRIBUTES 0x1 | (0x0 << 2) | (0x1 << 10) | (0x0 << 6) | (0x0 << 8)

#ifndef __ASSEMBLER__

#include "atomic.h"
#include "fs.h"
#include "function.h"
#include "hash.h"
#include "libk.h"
#include "stdint.h"

extern "C" void create_page_tables();
extern "C" void init_mmu();
extern "C" void switch_ttbr0(uint64_t pgd_paddr);

void patch_page_tables();

struct LocalPageLocation;
struct PageLocation;

struct PageTableLevel {
    uint64_t descriptors[TABLE_ENTRIES];
};

typedef PageTableLevel pgd_t;
typedef PageTableLevel pud_t;
typedef PageTableLevel pmd_t;
typedef PageTableLevel pte_t;

class PageTable {
   public:
    pgd_t* pgd;

    PageTable();

    ~PageTable();

    void map_vaddr(uint64_t vaddr, uint64_t paddr, uint64_t page_attributes, Function<void()> w);
    void use_page_table();
    bool unmap_vaddr(uint64_t vaddr);

   private:
    void alloc_pgd(Function<void()> w);

    void map_vaddr_pgd(uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                       Function<void()> w);

    void map_vaddr_pud(pud_t* pud, uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                       Function<void()> w);

    void map_vaddr_pmd(pmd_t* pmd, uint64_t vaddr, uint64_t paddr, uint64_t page_attributes,
                       Function<void()> w);

    void map_vaddr_pte(pte_t* pte, uint64_t vaddr, uint64_t paddr, uint64_t page_attributes);

    void free_pgd();

    void free_pud(pud_t* pud);

    void free_pmd(pmd_t* pmd);

    void free_pte(pte_t* pte);
};

enum LocationType { FILESYSTEM, SWAP, UNBACKED };

enum PageSharingMode { SHARED, PRIVATE };

struct SwapLocation {
    uint64_t swap_id;
    // uint64_t hw_id;
};

struct FileLocation {
    char* file_name;
    uint64_t offset;

    FileLocation(char* name, uint64_t off) {
        int path_length = K::strnlen(name, PATH_MAX - 1);
        file_name = (char*)kmalloc(path_length + 1);
        K::strncpy(file_name, name, PATH_MAX);

        offset = off;
    }

    ~FileLocation() {
        kfree(file_name);
    }
};

/**
 * Lock Ordering:
 *
 * (1) Supplemental Page Table Lock - protects all child LocalPageLocation and
 *                  ONLY the pointers to the PageLocation
 *
 * (2) Page Location Lock - protects all PageLocation data structures and all
 *                          child LocalPageLocation data structures, except
 *                          child's pointer to the page location.
 *
 * Never change the PageLocation* of a LocalPageLocation unless you have its
 *  parent Supplemental Page Table Lock
 */

/**
 * These are local to a processes Supplemental Page Table, holds the specific
 * qualities of that page for a given process, as well as the PageLocation.
 */
struct LocalPageLocation {
    // Lock location_lock; not needed?
    int perm;
    PageSharingMode sharing_mode;
    // uint64_t tid; some way to know which process out of the PageLocation->users is the one we
    // faulted on
    PageLocation* location;
    LocalPageLocation* next;
};

/**
 * holds the locations of pages, any changes to this struct must be reflected
 * in the pagecache, these are shared between different processes that share
 * pages in memory for any reason. (Have shared memory regions, share a common
 * file that is not edited, shared object files)
 */
struct PageLocation {
    Lock lock;
    LocationType location_type;
    bool owned; /* not used right now, but will indicate if this machine ownes the page */
    // bool dirty;
    bool present;
    uint64_t paddr;
    int ref_count;
    LocalPageLocation* users;
    union location {
        SwapLocation* swap;
        FileLocation* filesystem;
    } location;

    ~PageLocation() {
        if (location_type == FILESYSTEM) {
            delete location.filesystem;
        } else if (location_type == SWAP) {
            delete location.swap;
        }
    }
};

class SupplementalPageTable {
   public:
    HashMap<LocalPageLocation*> map;
    Lock lock;  // only lock the map with this, Page location and LocalPageLocation are locked
                // locally;

    SupplementalPageTable() : map(100) {
    }

    ~SupplementalPageTable() {
        // TODO: go through and clear all local pages + 0 ref page_locs
    }

    LocalPageLocation* vaddr_mapping(uint64_t vaddr);
    void map_vaddr(uint64_t vaddr, LocalPageLocation* local);
};

PageTableLevel* descriptor_to_vaddr(uint64_t descriptor);
uint64_t descriptor_to_paddr(uint64_t descriptor);

uint64_t get_pgd_index(uint64_t vaddr);
uint64_t get_pud_index(uint64_t vaddr);
uint64_t get_pmd_index(uint64_t vaddr);
uint64_t get_pte_index(uint64_t vaddr);

uint64_t paddr_to_table_descriptor(uint64_t paddr);
uint64_t paddr_to_block_descriptor(uint64_t paddr, uint64_t page_attributes);

uint64_t paddr_to_vaddr(uint64_t paddr);
uint64_t vaddr_to_paddr(uint64_t vaddr);

uint64_t build_page_attributes(LocalPageLocation* local);

#endif /*__ASSEMBLER__*/

#endif /*_VM_H*/
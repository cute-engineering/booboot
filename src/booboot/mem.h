#pragma once

#include "utils.h"
#include <stdint.h>

#define PAGE_SIZE (kib$(4))

#define pmlx_get_index$(addr, level) (((uint64_t)addr & ((uint64_t)0x1ff << (12 + level * 9))) >> (12 + level * 9))
#define page_get_phys$(x)            (x & 0x000ffffffffff000)

enum pml_fields : uint64_t
{
    PAGE_PRESENT = 1 << 0,
    PAGE_WRITABLE = 1 << 1,
    PAGE_USER = 1 << 2,
    PAGE_WRITE_THROUGH = 1 << 3,
    PAGE_NO_CACHE = 1 << 4,
    PAGE_ACCESSED = 1 << 5,
    PAGE_DIRTY = 1 << 6,
    PAGE_HUGE = 1 << 7,
    PAGE_GLOBAL = 1 << 8,
    PAGE_NO_EXECUTE = (uint64_t)1 << 63,
};

uintptr_t *_Nullable pml4(void);
uintptr_t paging_alloc(size_t count);
void paging_map_range(uintptr_t *_Nonnull pml4, uint64_t virt, uint64_t phys, size_t len, int64_t flags);
void paging_init(void);
uintptr_t alloc_stack(void);
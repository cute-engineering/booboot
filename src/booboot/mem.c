#include <handover/handover.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tiny-efi/efi.h>

#include "file.h"
#include "mem.h"

static uintptr_t *_Nullable _pml4 = NULL;

uintptr_t paging_alloc(size_t count)
{
    uintptr_t pages;
    efi_assert_success(efi_st()->boot_services->allocate_pages(ALLOCATE_ANY_PAGES, EFI_LOADER_DATA, count, &pages));
    memset((void *)pages, 0, count * PAGE_SIZE);
    return pages;
}

static uintptr_t paging_get_or_alloc(uintptr_t *pml, size_t index)
{
    if ((pml[index] & PAGE_PRESENT) == 0)
    {
        pml[index] = paging_alloc(1) | PAGE_PRESENT | PAGE_WRITABLE;
    }

    return page_get_phys$(pml[index]);
}

static void paging_map(uintptr_t *_Nonnull pml4, uint64_t virt, uint64_t phys, int64_t flags)
{
    size_t pml1_entry = pmlx_get_index$(virt, 0);
    size_t pml2_entry = pmlx_get_index$(virt, 1);
    size_t pml3_entry = pmlx_get_index$(virt, 2);
    size_t pml4_entry = pmlx_get_index$(virt, 3);

    uintptr_t *pml3 = (uintptr_t *)paging_get_or_alloc(pml4, pml4_entry);
    uintptr_t *pml2 = (uintptr_t *)paging_get_or_alloc(pml3, pml3_entry);
    uintptr_t *pml1 = (uintptr_t *)paging_get_or_alloc(pml2, pml2_entry);

    if (pml1[pml1_entry] & PAGE_PRESENT)
    {
        error$("page with virtual address 0x%llx is already mapped to %llx", virt, page_get_phys$(pml1[pml1_entry]));
        for (;;)
        {
            __asm__("hlt");
        }
    }

    pml1[pml1_entry] = phys | flags;
}

void paging_map_range(uintptr_t *_Nonnull pml4, uint64_t virt, uint64_t phys, size_t len, int64_t flags)
{
    for (size_t i = 0; i < len; i += PAGE_SIZE)
    {
        paging_map(pml4, virt + i, phys + i, flags);
    }
}

void paging_init(void)
{
    _pml4 = (uintptr_t *)paging_alloc(1);

    debug$("mapping boot-agent image...");
    paging_map_range(
        _pml4,
        (uint64_t)efi_image()->image_base,
        (uint64_t)efi_image()->image_base,
        efi_image()->image_size,
        PAGE_PRESENT | PAGE_WRITABLE);

    debug$("mapping first 4Gib of memory...");
    paging_map_range(
        _pml4,
        HANDOVER_UPPER_HALF + PAGE_SIZE,
        PAGE_SIZE,
        gib$(4) - PAGE_SIZE,
        PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE);
}

uintptr_t *_Nullable pml4(void)
{
    return _pml4;
}

uintptr_t alloc_stack(void)
{
    return paging_alloc(16);
}
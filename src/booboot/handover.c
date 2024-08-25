#define HANDOVER_INCLUDE_UTILITES

#include <handover/handover.h>
#include <logging>

#include "config.h"
#include "handover.h"
#include "loader.h"
#include "mem.h"

extern void __handoverEnterKernel(uintptr_t entry, uintptr_t handover, uintptr_t stack, uintptr_t pml4);

static size_t handover_add_string(HandoverPayload *handover, const char *str)
{
    size_t len = strlen(str) + 1;
    size_t offset = handover->size - len;
    memset((void *)((uintptr_t)handover + offset), 0, len);
    memcpy((void *)((uintptr_t)handover + offset), str, len);
    handover->size -= len;
    return offset;
}

void handover_apply(uintptr_t entry, uintptr_t stack)
{
    debug$("applying handover protocol...");

    uintptr_t payload = paging_alloc(kib$(16) / PAGE_SIZE);
    HandoverPayload *handover = (HandoverPayload *)payload;

    handover->magic = HANDOVER_COOLBOOT;
    handover->size = kib$(16);
    handover->agent = handover_add_string(handover, "Booboot");

    handover_append(
        handover,
        (HandoverRecord){
            .tag = HANDOVER_SELF,
            .flags = 0,
            .start = payload,
            .size = kib$(16),
        });

    handover_append(
        handover,
        (HandoverRecord){
            .tag = HANDOVER_STACK,
            .flags = 0,
            .start = stack,
            .size = 16 * PAGE_SIZE,
        });

    size_t file_name = handover_add_string(handover, selected_entry().kernel);
    handover_append(
        handover,
        (HandoverRecord){
            .tag = HANDOVER_FILE,
            .flags = 0,
            .start = (uintptr_t)ehdr(),
            .size = ehdr_len(),
            .file = {
                .name = file_name,
            },
        });

    size_t size;
    size_t desc_size;
    EfiMemoryDescriptor *memory_map = efi_mmap_snapshot(&size, &desc_size);

    for (size_t i = 0; i < (size / desc_size); i++)
    {
        EfiMemoryDescriptor *desc = (EfiMemoryDescriptor *)((uintptr_t)memory_map + i * desc_size);
        HandoverRecord record;

        switch (desc->type)
        {
        case EFI_LOADER_CODE:
        case EFI_LOADER_DATA:
        case EFI_BOOT_SERVICES_CODE:
        case EFI_BOOT_SERVICES_DATA:
        case EFI_RUNTIME_SERVICES_CODE:
        case EFI_RUNTIME_SERVICES_DATA:
        {
            record = (HandoverRecord){
                .tag = HANDOVER_LOADER,
                .flags = 0,
                .start = desc->physical_start,
                .size = desc->num_pages * PAGE_SIZE,
            };
            break;
        }

        case EFI_CONVENTIONAL_MEMORY:
        {
            record = (HandoverRecord){
                .tag = HANDOVER_FREE,
                .flags = 0,
                .start = desc->physical_start,
                .size = desc->num_pages * PAGE_SIZE,
            };
            break;
        }

        default:
        {
            record = (HandoverRecord){
                .tag = HANDOVER_RESERVED,
                .flags = 0,
                .start = desc->physical_start,
                .size = desc->num_pages * PAGE_SIZE,
            };
            break;
        }
        }

        handover_append(handover, record);
    }

    HandoverRequest *reqs = (HandoverRequest *)load_section(HANDOVER_SECTION);

    for (size_t i = 0; reqs[i].tag != (uint32_t)HANDOVER_END; i++)
    {
        debug$("client is requesting %s", handover_tag_name(reqs[i].tag));
        switch (reqs[i].tag)
        {
        case HANDOVER_MAGIC:
        {
            handover_insert(
                handover,
                0,
                (HandoverRecord){
                    .tag = HANDOVER_MAGIC,
                    .flags = 0,
                    .start = 0,
                    .size = 0,
                });
            break;
        }
        case HANDOVER_CMDLINE:
        {
            handover_insert(
                handover,
                handover->count,
                (HandoverRecord){
                    .tag = HANDOVER_CMDLINE,
                    .flags = 0,
                    .start = handover_add_string(handover, selected_entry().cmdline),
                    .size = strlen(selected_entry().cmdline) + 1,
                });

            break;
        }
        default:
        {
            error$("unsupported request %s", handover_tag_name(reqs[i].tag));
            for (;;)
            {
                __asm__("hlt");
            }
        }
        }
    }

    handover_insert(
        handover,
        handover->count,
        (HandoverRecord){
            .tag = HANDOVER_END,
            .flags = 0,
            .start = 0,
            .size = 0,
        });

    debug$("entry: %llx", entry);
    uintptr_t sp = stack + 16 * PAGE_SIZE + HANDOVER_UPPER_HALF;

    efi_deinit();

    __handoverEnterKernel(entry, payload + HANDOVER_UPPER_HALF, sp, (uintptr_t)pml4());
}

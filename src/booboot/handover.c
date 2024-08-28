#define HANDOVER_INCLUDE_UTILITES

#include <handover/handover.h>
#include <logging>
#include <tiny-efi/efi.h>

#include "config.h"
#include "file.h"
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

    HandoverRequest *_Nullable reqs = (HandoverRequest *)load_section(HANDOVER_SECTION);
    if (reqs != NULL)
    {

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
                        .start = (uintptr_t)selected_entry().cmdline,
                        .size = strlen(selected_entry().cmdline) + 1,
                    });

                break;
            }
            case HANDOVER_FB:
            {
                EfiGuid guid = EFI_GOP_GUID;
                EfiGop *gop;
                size_t infoSize;
                EfiGopModeInfo *info;

                efi_assert_success(efi_st()->boot_services->locate_protocol(&guid, NULL, (void **)&gop));
                efi_assert_success(gop->query_mode(gop, gop->mode->max_mode - 1, &infoSize, &info));
                efi_assert_success(gop->set_mode(gop, gop->mode->max_mode - 1));

                debug$("setting GOP mode to %dx%d", info->horizontal_resolution, info->vertical_resolution);

                handover_insert(
                    handover,
                    handover->count,
                    (HandoverRecord){
                        .tag = HANDOVER_FB,
                        .flags = 0,
                        .start = gop->mode->framebuffer_base,
                        .size = gop->mode->framebuffer_size,
                        .fb = {
                            .width = gop->mode->info->horizontal_resolution,
                            .height = gop->mode->info->vertical_resolution,
                            .pitch = gop->mode->info->pixels_per_scan_line * sizeof(uint32_t),
                            .format = HANDOVER_BGRX8888,
                        }});

                break;
            }
            case HANDOVER_FILE:
            {
                size_t length;
                for (size_t i = 0; i < selected_entry().modules.len; i++)
                {
                    debug$("loading module %s", selected_entry().modules.buf[i].string);
                    char const *content = efi_read_file(selected_entry().modules.buf[i].string, &length);
                    handover_append(
                        handover,
                        (HandoverRecord){
                            .tag = HANDOVER_FILE,
                            .flags = 0,
                            .start = (uintptr_t)content,
                            .size = length,
                            .file = {
                                .name = handover_add_string(handover, selected_entry().modules.buf[i].string),
                            },
                        });
                }

                break;
            }

            case HANDOVER_RSDP:
            {
                handover_append(
                    handover,
                    (HandoverRecord){
                        .tag = HANDOVER_RSDP,
                        .flags = 0,
                        .start = uefi_find_rsdp(),
                        .size = 0x1000,
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

#include <logging>
#include <tiny-efi/efi.h>

#include "config.h"
#include "file.h"
#include "loader.h"
#include "mem.h"
#include "protocols.h"
#include "utils.h"

int32_t _fltused = 0;
extern void __enterKernel(uintptr_t entry, uintptr_t handover, uintptr_t stack, uintptr_t pml4);

static void menu(void)
{
    if (config_entries_count() == 0)
    {
        error$("no entries found in the configuration file.");
        return;
    }
    else if (config_entries_count() == 1)
    {
        efi_assert_success(config_entry(0));
    }
    else
    {
        // TODO
        __builtin_unreachable();
    }
}

EfiStatus efi_main(EfiHandle handle, EfiSystemTable *st)
{
    efi_init(handle, st);

    efi_assert_success(efi_st()->boot_services->set_watchdog_timer(0, 0, 0, NULL));

    efi_console_clear();
    efi_console_reset();

    info$("booting from Booboot...");

    char *_Nonnull cfg = efi_read_file("loader.json", NULL);
    efi_assert_success(config_parse(cfg));

    menu();
    paging_init();

    uintptr_t entry = load_binary(selected_entry().kernel);
    uintptr_t stack = alloc_stack();
    apply_protocol(selected_entry().protocol, entry, stack);

    for (;;)
    {
        __asm__("hlt");
    }

    return EFI_SUCCESS;
}
#pragma once

#include <logging>
#include <tiny-efi/efi.h>

#define efi_assert_success(status) ({                                       \
    if (status != EFI_SUCCESS)                                              \
    {                                                                       \
        error$("Assertion failed: %s:%d (%d)", __FILE__, __LINE__, status); \
        for (;;)                                                            \
        {                                                                   \
            __asm__("hlt");                                                 \
        }                                                                   \
    }                                                                       \
})

#define kib$(x) ((uintptr_t)(x) * 1024)

#define mib$(x) (kib$(x) * 1024)

#define gib$(x) (mib$(x) * 1024)

#define align_up$(x, align) (((x) + (align) - 1) & ~((align) - 1))

#define align_down$(x, align) ((x) & ~((align) - 1))

uintptr_t uefi_find_rsdp(void);
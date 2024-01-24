#pragma once

#include <ce-efi/base.h>
#include <ce-panic.h>

#define efi_assert_success(status) ({ if (EFI_ERROR(status)) { ce_panic("efi error"); } })

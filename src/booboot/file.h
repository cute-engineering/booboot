#pragma once

#include <stddef.h>
#include <tiny-efi/efi.h>

EfiLoadedImage *_Nonnull efi_image(void);

EfiSfsp *_Nonnull efi_rootfs(void);

EfiFp *_Nonnull efi_rootdir(void);

char *_Nonnull efi_read_file(uint16_t *_Nonnull path, size_t *_Nullable len);
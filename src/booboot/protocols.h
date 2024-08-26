#pragma once

#include <stdint.h>

typedef struct
{
    uint64_t width;
    uint64_t height;
    uint64_t bpp;
} Resolution;

uintptr_t apply_protocol(char const *_Nonnull protocol_name, uintptr_t entry, uintptr_t stack);
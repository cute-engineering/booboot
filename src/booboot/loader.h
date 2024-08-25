#pragma once

#include <elf>
#include <stddef.h>

void *_Nullable ehdr(void);

size_t ehdr_len(void);

uintptr_t load_binary(char const *_Nonnull path);

void *_Nonnull load_section(char const *_Nonnull name);
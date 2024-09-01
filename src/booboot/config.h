#pragma once

#include <tiny-efi/efi.h>
#include <tiny-json/json.h>

typedef struct
{
    char const *_Nonnull label;
    char const *_Nonnull kernel;
    char const *_Nonnull protocol;
    char const *_Nonnull cmdline;
    json_vec_t modules;
} Entry;

EfiStatus config_parse(char const *_Nonnull json);

EfiStatus config_entry(size_t index);

size_t config_entries_count(void);

Entry selected_entry(void);

bool is_verbose(void);
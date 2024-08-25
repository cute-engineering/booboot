#include <string.h>

#include "config.h"
#include "utils.h"

static json_t entries;
static Entry _selected_entry;

void *_Nonnull _json_realloc(void *_Nullable ptr, size_t size)
{
    void *ret = NULL;
    efi_assert_success(efi_st()->boot_services->allocate_pool(EFI_LOADER_DATA, size, &ret));
    if (ptr != NULL)
    {
        memcpy(ret, ptr, size);
        efi_assert_success(efi_st()->boot_services->free_pool(ptr));
    }

    return ret;
}

void _json_free(void *_Nonnull ptr, [[gnu::unused]] size_t size)
{
    efi_assert_success(efi_st()->boot_services->free_pool(ptr));
}

EfiStatus config_parse(char const *json)
{
    json_reader_t reader = json_init(json, strlen(json));
    json_t cfg = json_parse(&reader);

    if (cfg.type == JSON_ERROR)
    {
        return EFI_INVALID_PARAMETER;
    }

    entries = json_get(cfg, "entries");
    if (entries.type != JSON_ARRAY)
    {
        error$("invalid entries in config, entries must be an array");
        return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
}

size_t config_entries_count(void)
{
    return entries.array.len;
}

EfiStatus config_entry(size_t index)
{
    json_t raw_entry = entries.array.buf[index];

    if (raw_entry.type != JSON_OBJECT)
    {
        error$("invalid entry in config, entry must be an object");
        return EFI_PROTOCOL_ERROR;
    }

    json_t raw_label = json_get(raw_entry, "label");
    if (raw_label.type != JSON_STRING)
    {
        error$("invalid label in config, label must be a string");
        return EFI_PROTOCOL_ERROR;
    }

    json_t raw_kernel = json_get(raw_entry, "kernel");
    if (raw_kernel.type != JSON_STRING)
    {
        error$("invalid kernel in config, kernel must be a string");
        return EFI_PROTOCOL_ERROR;
    }

    json_t raw_protocol = json_get(raw_entry, "protocol");
    if (raw_protocol.type != JSON_STRING)
    {
        error$("invalid protocol in config, protocol must be a string");
        return EFI_PROTOCOL_ERROR;
    }

    json_t raw_cmdline = json_get(raw_entry, "cmdline");
    if (raw_cmdline.type != JSON_STRING)
    {
        error$("invalid cmdline in config, cmdline must be a string");
        return EFI_PROTOCOL_ERROR;
    }

    json_t raw_modules = json_get(raw_entry, "modules");
    if (raw_modules.type != JSON_ARRAY)
    {
        error$("invalid modules in config, modules must be an array");
        return EFI_PROTOCOL_ERROR;
    }

    for (size_t i = 0; i < raw_modules.array.len; i++)
    {
        json_t raw_module = raw_modules.array.buf[i];
        if (raw_module.type != JSON_STRING)
        {
            error$("invalid module items in config, must be strings");
            return EFI_PROTOCOL_ERROR;
        }
    }

    _selected_entry = (Entry){
        .label = raw_label.string,
        .kernel = raw_kernel.string,
        .cmdline = raw_cmdline.string,
        .modules = raw_modules.array,
    };

    debug$("loading entry: %s", _selected_entry.label);
    return EFI_SUCCESS;
}

Entry selected_entry(void)
{
    return _selected_entry;
}
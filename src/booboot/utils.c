#include <logging>
#include <string.h>
#include <tiny-efi/efi.h>

uintptr_t uefi_find_rsdp(void)
{
    EfiGuid acpi = ACPI_TABLE_GUID;
    EfiGuid acpi2 = ACPI2_TABLE_GUID;
    void *acpi_table = NULL;

    for (size_t i = 0; i < efi_st()->num_table_entries; i++)
    {
        if (memcmp(&efi_st()->config_table[i].vendor_guid, &acpi, sizeof(EfiGuid)) == 0)
        {
            acpi_table = efi_st()->config_table[i].vendor_table;
            break;
        }
        else if (memcmp(&efi_st()->config_table[i].vendor_guid, &acpi2, sizeof(EfiGuid)) == 0)
        {
            acpi_table = efi_st()->config_table[i].vendor_table;
            break;
        }
    }

    debug$("ACPI table at %p", acpi_table);
    return (uintptr_t)acpi_table;
}
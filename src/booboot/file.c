#include <string.h>
#include <tiny-efi/efi.h>

#include "file.h"
#include "utils.h"

static EfiLoadedImage *_Nullable _loaded_image = NULL;
static EfiSfsp *_Nullable _rootfs = NULL;
static EfiFp *_Nullable _rootdir = NULL;

EfiLoadedImage *_Nonnull efi_image(void)
{
    if (_loaded_image != NULL)
    {
        return _loaded_image;
    }

    EfiGuid lip_guid = EFI_LIP_GUID;

    efi_assert_success(efi_st()->boot_services->open_protocol(
        efi_handle(),
        &lip_guid,
        (void **)&_loaded_image,
        efi_handle(),
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL));

    return _loaded_image;
}

EfiSfsp *_Nonnull efi_rootfs(void)
{
    if (_rootfs != NULL)
    {
        return _rootfs;
    }

    EfiGuid guid = EFI_SFSP_GUID;

    efi_assert_success(efi_st()->boot_services->open_protocol(
        efi_image()->device_handle,
        &guid,
        (void **)&_rootfs,
        efi_handle(),
        NULL,
        EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL));

    return _rootfs;
}

EfiFp *_Nonnull efi_rootdir(void)
{
    if (_rootdir != NULL)
    {
        return _rootdir;
    }

    efi_assert_success(efi_rootfs()->open_volume(efi_rootfs(), &_rootdir));

    return _rootdir;
}

char *_Nonnull efi_read_file(uint16_t *_Nonnull path, size_t *_Nullable len)
{
    EfiFp *file;

    EfiStatus status =
        efi_rootdir()->open(efi_rootdir(), &file, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

    if (status != EFI_SUCCESS)
    {
        error$("failed to open file: ");
        efi_console_write(path);
        for (;;)
        {
            __asm__("hlt");
        }
    }

    efi_assert_success(status);

    EfiGuid guid = EFI_FILE_INFO_ID;
    EfiFileInfo info;

    uint64_t size = sizeof(info);
    file->get_info(file, &guid, &size, &info);

    char *buf;
    efi_assert_success(
        efi_st()->boot_services->allocate_pool(EFI_LOADER_DATA, info.file_size + 1, (void **)&buf));

    efi_assert_success(file->read(file, &info.file_size, buf));
    buf[info.file_size] = '\0';

    if (len != NULL)
    {
        *len = info.file_size;
    }

    return buf;
}
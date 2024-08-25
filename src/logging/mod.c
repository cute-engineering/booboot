#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOFLOAT

#include "mod.h"
#include "stb_sprintf.h"

void log_impl(int level, char const *_Nonnull fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    efi_st()->console_out->set_attribute(efi_st()->console_out, level_colors[level]);
    efi_console_write(level_names[level]);
    efi_console_write(L" ");
    efi_st()->console_out->set_attribute(efi_st()->console_out, EFI_LIGHTGRAY);

    char buffer[1024];
    stbsp_vsprintf(buffer, fmt, args);

    for (size_t i = 0; i < 1024; i++)
    {
        if (buffer[i] == '\0')
        {
            break;
        }
        else if (buffer[i] == '\n')
        {
            efi_console_write(L"\r\n");
            continue;
        }

        uint16_t buf[2] = {buffer[i], 0};
        efi_console_write(buf);
    }

    efi_console_write(L"\r\n");

    va_end(args);
}
#include <stdarg.h>
#include <stddef.h>
#include <tiny-efi/efi.h>

enum
{
    LOG_NONE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_LENGTH
};

static uint16_t *_Nonnull level_names[LOG_LENGTH] = {
    [LOG_NONE] = L"",
    [LOG_DEBUG] = L"DEBUG",
    [LOG_INFO] = L"INFO",
    [LOG_WARN] = L"WARN",
    [LOG_ERROR] = L"ERROR",
};

static size_t level_colors[LOG_LENGTH] = {
    [LOG_NONE] = EFI_LIGHTGRAY,
    [LOG_DEBUG] = EFI_LIGHTGREEN,
    [LOG_INFO] = EFI_BLUE,
    [LOG_WARN] = EFI_YELLOW,
    [LOG_ERROR] = EFI_RED,
};

void log_impl(int level, char const *_Nonnull fmt, ...);

#ifdef __ck_debug__
#    define debug$(...) log_impl(LOG_DEBUG, __VA_ARGS__)
#else
#    define debug$(...) ((void)0)
#endif
#define info$(...)  log_impl(LOG_INFO, __VA_ARGS__)
#define warn$(...)  log_impl(LOG_WARN, __VA_ARGS__)
#define error$(...) log_impl(LOG_ERROR, __VA_ARGS__)
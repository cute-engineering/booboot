#include <handover/builder.h>
#include <logging>
#include <string.h>

#include "handover.h"
#include "protocols.h"

uintptr_t apply_protocol(char const *_Nonnull protocol_name, uintptr_t entry, uintptr_t stack)
{
    if (strcmp(protocol_name, "handover"))
    {
        handover_apply(entry, stack);
    }
    else
    {
        error$("unknown protocol: %s", protocol_name);
        for (;;)
        {
            __asm__("hlt");
        }
    }

    return 0;
}
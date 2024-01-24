#include <ce-efi/st.h>

#include "base.h"

void ce_panic(ce_cstr)
{
    while (1)
        ;
}

efi_status efi_main(efi_handle, efi_system_table *st)
{
    efi_assert_success(st->boot_services->set_watchdog_timer(0, 0, 0, NULL));
    efi_assert_success(st->con_out->reset(st->con_out, 1));
    efi_assert_success(st->con_out->output_string(st->con_out, u"Hello, world!\r\n"));
    return EFI_SUCCESS;
}

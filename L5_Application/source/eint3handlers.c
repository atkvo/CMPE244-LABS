#ifdef __cplusplus
extern "C" {
#endif

#include "eint3handlers.h"
#include "eint.h"
#include "printf_lib.h"

void callback1(void)
{
    u0_dbg_printf("cb1\n");
}

void callback2(void)
{
    u0_dbg_printf("cb2\n");
}

void setup_eint3_interrupts()
{
    eint3_enable_port2(3, eint_rising_edge, &callback2);
}

#ifdef __cplusplus
}
#endif
#include "hal_data.h"
#include "m33_services.h"

void hal_entry(void)
{
    if (FSP_SUCCESS != m33_services_init())
    {
        while (1)
        {
            __WFI();
        }
    }

    m33_services_run();
}

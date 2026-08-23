#ifndef M33_SERVICES_H
#define M33_SERVICES_H

#include "hal_data.h"

fsp_err_t m33_services_init(void);
void m33_services_run(void);
void m33_services_ipc_callback(ipc_callback_args_t * p_args);

#endif

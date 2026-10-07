#ifndef APP_THREADX_H
#define APP_THREADX_H

#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Watch these counters in the debugger to verify both threads are scheduled. */
extern volatile ULONG heartbeat_250ms_count;
extern volatile ULONG heartbeat_1s_count;
extern volatile UINT app_threadx_last_error;
extern TX_THREAD * volatile app_threadx_stack_error_thread;

VOID tx_application_define(VOID *first_unused_memory);

#ifdef TX_ENABLE_STACK_CHECKING
VOID app_threadx_stack_error(TX_THREAD *thread_ptr);
#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_THREADX_H */

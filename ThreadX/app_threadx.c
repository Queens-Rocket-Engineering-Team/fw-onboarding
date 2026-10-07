#include "app_threadx.h"

#include "main.h"

#define APP_THREAD_STACK_BYTES 1024U

static TX_THREAD heartbeat_250ms_thread;
static TX_THREAD heartbeat_1s_thread;
_Alignas(8) static UCHAR heartbeat_250ms_stack[APP_THREAD_STACK_BYTES];
_Alignas(8) static UCHAR heartbeat_1s_stack[APP_THREAD_STACK_BYTES];

volatile ULONG heartbeat_250ms_count;
volatile ULONG heartbeat_1s_count;
volatile UINT app_threadx_last_error = TX_SUCCESS;
TX_THREAD * volatile app_threadx_stack_error_thread;

static VOID check_threadx_status(UINT status)
{
    if (status != TX_SUCCESS)
    {
        app_threadx_last_error = status;
        Error_Handler();
    }
}

static VOID heartbeat_250ms_entry(ULONG thread_input)
{
    (void)thread_input;

    for (;;)
    {
        ++heartbeat_250ms_count;
        check_threadx_status(tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 4U));
    }
}

static VOID heartbeat_1s_entry(ULONG thread_input)
{
    (void)thread_input;

    for (;;)
    {
        ++heartbeat_1s_count;
        check_threadx_status(tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND));
    }
}

#ifdef TX_ENABLE_STACK_CHECKING
VOID app_threadx_stack_error(TX_THREAD *thread_ptr)
{
    app_threadx_stack_error_thread = thread_ptr;
    Error_Handler();
}
#endif

VOID tx_application_define(VOID *first_unused_memory)
{
    /* All application objects and stacks are static; no byte pool is needed. */
    (void)first_unused_memory;

#ifdef TX_ENABLE_STACK_CHECKING
    check_threadx_status(tx_thread_stack_error_notify(app_threadx_stack_error));
#endif

    check_threadx_status(tx_thread_create(&heartbeat_250ms_thread,
                                         "Heartbeat 250 ms",
                                         heartbeat_250ms_entry,
                                         0UL,
                                         heartbeat_250ms_stack,
                                         sizeof(heartbeat_250ms_stack),
                                         10U,
                                         10U,
                                         TX_NO_TIME_SLICE,
                                         TX_AUTO_START));

    check_threadx_status(tx_thread_create(&heartbeat_1s_thread,
                                         "Heartbeat 1 s",
                                         heartbeat_1s_entry,
                                         0UL,
                                         heartbeat_1s_stack,
                                         sizeof(heartbeat_1s_stack),
                                         20U,
                                         20U,
                                         TX_NO_TIME_SLICE,
                                         TX_AUTO_START));
}

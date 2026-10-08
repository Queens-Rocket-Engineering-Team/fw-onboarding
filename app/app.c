/* app.c - ThreadX application entry for the STINGER altimeter template.
 *
 * tx_kernel_enter() (called from main) calls tx_application_define() once,
 * then starts the scheduler. Create every thread, queue and mutex here.
 */
#include "tx_api.h"
#include "main.h"   /* LED_Pin, LED_GPIO_Port from the CubeMX user label */
#include "usart.h"  /* huart1 */

#define HEARTBEAT_STACK_BYTES   1024U
#define HEARTBEAT_PRIORITY      10U

static TX_THREAD heartbeat_thread;
/* Static stack: its size shows up in the .map file, unlike a byte pool. */
static ULONG heartbeat_stack[HEARTBEAT_STACK_BYTES / sizeof(ULONG)];

static void heartbeat_entry(ULONG input)
{
    (void)input;

    static const char banner[] = "STINGER: ThreadX running\r\n";
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)banner, sizeof(banner) - 1U, 100U);

    for (;;) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        /* 500 ms: ten toggles should take exactly 5 s on a stopwatch. */
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2U);
    }
}

void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    (void)tx_thread_create(&heartbeat_thread, "heartbeat", heartbeat_entry, 0U,
                           heartbeat_stack, sizeof(heartbeat_stack),
                           HEARTBEAT_PRIORITY, HEARTBEAT_PRIORITY,
                           TX_NO_TIME_SLICE, TX_AUTO_START);
}
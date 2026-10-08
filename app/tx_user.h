/* tx_user.h - ThreadX configuration for QRET STM32 firmware.
 * Passed to the ThreadX build via TX_USER_FILE (see cmake/threadx.cmake).
 */
#ifndef TX_USER_H
#define TX_USER_H

/* 1 ms tick. Must match SYSTICK_CYCLES in app/tx_initialize_low_level.S. */
#define TX_TIMER_TICKS_PER_SECOND   1000U

/* Detect thread stack overflow. Leave on until stack sizes are proven. */
#define TX_ENABLE_STACK_CHECKING

#endif /* TX_USER_H */
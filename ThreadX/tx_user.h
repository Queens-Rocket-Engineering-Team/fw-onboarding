#ifndef TX_USER_H
#define TX_USER_H

/* The kernel SysTick is independent of the HAL's TIM2 timebase. */
#define TX_TIMER_TICKS_PER_SECOND 1000
#define TX_TIMER_THREAD_STACK_SIZE 1024

/* Apply this header to the application and the kernel with TX_INCLUDE_USER_DEFINE_FILE. */
#if defined(DEBUG)
#define TX_ENABLE_STACK_CHECKING
#endif

#endif /* TX_USER_H */

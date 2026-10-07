/*
 * Copyright (c) 2024 Microsoft Corporation
 * Copyright (c) 2026-present Eclipse ThreadX contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Adapted from the Cortex-M3/GNU ThreadX low-level initialization example
 * for the STM32Cube startup, linker symbols, and runtime core clock.
 */

#include "tx_user.h"

    .syntax unified
    .cpu cortex-m3
    .thumb

    .section .text._tx_initialize_low_level,"ax",%progbits
    .p2align 2
    .global _tx_initialize_low_level
    .type _tx_initialize_low_level,%function
    .thumb_func
_tx_initialize_low_level:
    /* ThreadX's first scheduler entry re-enables interrupts. */
    cpsid   i

    /* Application storage is static; still provide the normal unused-RAM hint. */
    ldr     r0, =_tx_initialize_unused_memory
    ldr     r1, =_end
    adds    r1, r1, #7
    bic     r1, r1, #7
    str     r1, [r0]

    /* Retain STM32Cube's complete vector table and reset-time MSP. */
    ldr     r0, =0xE000ED00
    ldr     r1, =g_pfnVectors
    str     r1, [r0, #0x08]       /* SCB->VTOR */

    /* Match the Cortex-M3 port: SVC/PendSV lowest, SysTick priority 4. */
    movs    r1, #0xFF
    strb    r1, [r0, #0x1F]       /* SVCall priority */
    strb    r1, [r0, #0x22]       /* PendSV priority */
    movs    r1, #0x40
    strb    r1, [r0, #0x23]       /* SysTick priority (4 implemented bits) */

    /* Stop the counter, then clear any stale tick before restarting it. */
    ldr     r1, =0xE000E010
    movs    r2, #0
    str     r2, [r1, #0x00]       /* SysTick->CTRL */
    str     r2, [r1, #0x08]       /* SysTick->VAL */
    ldr     r2, =0x02000000
    str     r2, [r0, #0x04]       /* SCB->ICSR: PENDSTCLR */
    mov     r0, r1

    ldr     r1, =SystemCoreClock
    ldr     r1, [r1]
    ldr     r2, =TX_TIMER_TICKS_PER_SECOND
    udiv    r1, r1, r2
    cbz     r1, .Linvalid_systick
    subs    r1, r1, #1
    ldr     r2, =0x00FFFFFF
    cmp     r1, r2
    bhi     .Linvalid_systick
    str     r1, [r0, #0x04]       /* SysTick->LOAD */
    movs    r1, #7                /* core clock, interrupt, enable */
    str     r1, [r0, #0x00]
    dsb
    isb
    bx      lr

.Linvalid_systick:
    b       Error_Handler
    .size _tx_initialize_low_level, .-_tx_initialize_low_level
    .ltorg

    .section .text.SysTick_Handler,"ax",%progbits
    .p2align 2
    .global SysTick_Handler
    .global __tx_SysTickHandler
    .type SysTick_Handler,%function
    .type __tx_SysTickHandler,%function
    .thumb_func
SysTick_Handler:
    .thumb_func
__tx_SysTickHandler:
    /* Preserve EXC_RETURN and maintain the eight-byte C-call stack alignment. */
    push    {r0, lr}
#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    bl      _tx_execution_isr_enter
#endif
    bl      _tx_timer_interrupt
#if defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)
    bl      _tx_execution_isr_exit
#endif
    pop     {r0, lr}
    bx      lr
    .size SysTick_Handler, .-SysTick_Handler
    .size __tx_SysTickHandler, .-__tx_SysTickHandler

    .section .note.GNU-stack,"",%progbits

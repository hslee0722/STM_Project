#include "device_driver.h"
#include "app_state.h"
#include <stdio.h>

void _Invalid_ISR(void)
{
    unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
    Uart2_Printf("\nInvalid_Exception: %d!\n", r);
    for (;;);
}

extern volatile int system_mode;
void EXTI15_10_IRQHandler(void)
{
    system_mode = (system_mode + 1) % 4;
    EXTI->PR = 0x1 << 13;
    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
}

void RTC_Alarm_IRQHandler(void)
{
    if (EXTI->PR & (1 << 17)) {
        pill_alarm_flag = 1;
        EXTI->PR |= (1 << 17);
        RTC->ISR &= ~(1 << 8);
    }
}

volatile unsigned char Uart_Data = 0;
static int uart2_rx_index = 0;

void USART2_IRQHandler(void)
{
    Uart_Data = USART2->DR & 0xff;

    if (uart2_rx_exist == 1) { NVIC_ClearPendingIRQ(USART2_IRQn); return; }

    if (Uart_Data == '\n' || Uart_Data == ' ') {
        uart2_buffer[uart2_rx_index] = '\0';
        uart2_rx_index = 0;
        uart2_rx_exist = 1;
    } else if (uart2_rx_index < 63) {
        uart2_buffer[uart2_rx_index++] = Uart_Data;
    }
    NVIC_ClearPendingIRQ(USART2_IRQn);
}

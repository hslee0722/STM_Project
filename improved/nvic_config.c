#include "device_driver.h"
#include "nvic_config.h"

void NVIC_Config(void)
{
    NVIC_SetPriorityGrouping(3);

    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(3, 0, 0));
    NVIC_SetPriority(USART1_IRQn,    NVIC_EncodePriority(3, 1, 0));
    NVIC_SetPriority(USART2_IRQn,    NVIC_EncodePriority(3, 2, 0));
    NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(3, 3, 0));
}

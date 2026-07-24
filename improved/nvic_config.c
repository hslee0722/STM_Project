#include "device_driver.h"
#include "nvic_config.h"

/*
 * 인터럽트 우선순위 (그룹 3 : preempt 4bit / sub 0bit)
 */
void NVIC_Config(void)
{
    NVIC_SetPriorityGrouping(3);

    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(3, 0, 0));  /* 복약 알람 */
    NVIC_SetPriority(USART1_IRQn,    NVIC_EncodePriority(3, 1, 0));  /* HC-06 앱 명령 */
    NVIC_SetPriority(USART2_IRQn,    NVIC_EncodePriority(3, 2, 0));  /* PC 디버그 */
}

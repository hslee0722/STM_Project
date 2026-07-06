#include "device_driver.h"
#include "nvic_config.h"

/*
 * NVIC_Config()
 * ------------------------------------------------------------
 * 프로젝트에서 사용하는 인터럽트 우선순위를 명확히 설정합니다.
 *
 * 우선순위 숫자가 낮을수록 더 높은 우선순위입니다.
 *
 * 추천 기준:
 * 1. RTC Alarm
 *    - 약 복용 시간 알림이므로 놓치면 안 됨
 *    - 단, ISR 안에서는 플래그만 세우고 실제 모터 제어는 main loop에서 수행
 *
 * 2. UART1 / UART2 RX
 *    - BLE 앱 명령과 PC 디버그 명령 수신
 *    - 수신 버퍼 유실 방지 목적
 *
 * 3. EXTI 버튼
 *    - 현재 PB5는 폴링 방식이라 EXTI를 쓰지 않음
 *    - 나중에 버튼 인터럽트로 바꿀 경우를 대비해 예시만 남김
 *
 * 주의:
 * 현재 프로젝트의 스텝모터는 blocking delay 기반이라
 * 타이머 인터럽트로 스텝 pulse를 만들지는 않습니다.
 * 추후 논블로킹 스텝모터로 개선한다면 TIM 인터럽트를 UART보다 높게 둘 수 있습니다.
 */
void NVIC_Config(void)
{
    /*
     * STM32F4는 우선순위 그룹 설정이 가능합니다.
     * 여기서는 preempt priority 4bit, subpriority 0bit 구조를 사용합니다.
     * CMSIS 함수가 사용 가능하다는 전제입니다.
     */
    NVIC_SetPriorityGrouping(3);

    /*
     * IRQ 번호:
     * USART1_IRQn       : 보통 37
     * USART2_IRQn       : 보통 38
     * RTC_Alarm_IRQn    : 보통 41
     * EXTI15_10_IRQn    : 보통 40
     *
     * 기존 코드에서는 숫자 직접 사용이 섞여 있었지만,
     * 가능하면 CMSIS의 IRQn 이름을 쓰는 것이 안전합니다.
     */
    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(3, 0, 0));
    NVIC_SetPriority(USART1_IRQn,    NVIC_EncodePriority(3, 1, 0));
    NVIC_SetPriority(USART2_IRQn,    NVIC_EncodePriority(3, 2, 0));
    NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(3, 3, 0));
}

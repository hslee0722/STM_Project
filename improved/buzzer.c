#include "device_driver.h"
#include "pin_map.h"

/* 패시브 부저 : PIN_BUZZER (PB6) 를 TIM4_CH1 PWM 으로 구동 */
void Buzzer_Init(void)
{
    /* 1. GPIO 및 TIM4 클럭 활성화 */
    Macro_Set_Bit(RCC->AHB1ENR, PIN_BUZZER_RCC_BIT);
    Macro_Set_Bit(RCC->APB1ENR, 2);    /* TIM4 */

    /* 2. 부저 핀을 Alternate Function 으로 */
    Macro_Write_Block(PIN_BUZZER_PORT->MODER, 0x3, 0x2,
                      PIN_POS2(PIN_BUZZER_NUM));
    Macro_Write_Block(PIN_BUZZER_PORT->AFR[PIN_AFR_IDX(PIN_BUZZER_NUM)],
                      0xF, PIN_BUZZER_AF, PIN_AFR_POS(PIN_BUZZER_NUM));

    /* 3. 1us 카운터 (84MHz 기준) */
    TIM4->PSC = 84 - 1;
    TIM4->ARR = 1000 - 1;
    TIM4->CCR1 = 0;                    /* Duty 0% = 무음 */

    /* 4. PWM 모드 1 */
    TIM4->CCMR1 &= ~(0xFF);
    TIM4->CCMR1 |= (0x6 << 4);
    TIM4->CCER  |= (1 << 0);

    /* 5. 타이머 시작 */
    TIM4->CR1 |= (1 << 0);
}

void Buzzer_On(void)
{
    /* 2000Hz : 1,000,000 / 2000 = 500 */
    TIM4->ARR  = 500 - 1;
    TIM4->CCR1 = 250;                  /* 50% 듀티 */
}

void Buzzer_Off(void)
{
    TIM4->CCR1 = 0;
}

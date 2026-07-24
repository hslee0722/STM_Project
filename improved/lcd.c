#include "device_driver.h"
#include "pin_map.h"

/*
 * 상태 표시 LED (Active Low : ODR=0 점등 / ODR=1 소등)
 *   RED   : 약 공급 · 복귀 등 "진행 중"
 *   GREEN : 수동 배출 · 컨베이어 전진 시작
 */
void Status_LED_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, PIN_STATUS_RED_RCC_BIT);
    Macro_Set_Bit(RCC->AHB1ENR, PIN_STATUS_GRN_RCC_BIT);

    Macro_Write_Block(PIN_STATUS_RED_PORT->MODER, 0x3, 0x1,
                      PIN_POS2(PIN_STATUS_RED_NUM));
    Macro_Clear_Bit(PIN_STATUS_RED_PORT->OTYPER, PIN_STATUS_RED_NUM);
    Macro_Write_Block(PIN_STATUS_RED_PORT->OSPEEDR, 0x3, 0x2,
                      PIN_POS2(PIN_STATUS_RED_NUM));

    Macro_Write_Block(PIN_STATUS_GRN_PORT->MODER, 0x3, 0x1,
                      PIN_POS2(PIN_STATUS_GRN_NUM));
    Macro_Clear_Bit(PIN_STATUS_GRN_PORT->OTYPER, PIN_STATUS_GRN_NUM);
    Macro_Write_Block(PIN_STATUS_GRN_PORT->OSPEEDR, 0x3, 0x2,
                      PIN_POS2(PIN_STATUS_GRN_NUM));

    Status_LED_All_Off();
}

void Status_LED_All_Off(void)
{
    Macro_Set_Bit(PIN_STATUS_RED_PORT->ODR, PIN_STATUS_RED_NUM);
    Macro_Set_Bit(PIN_STATUS_GRN_PORT->ODR, PIN_STATUS_GRN_NUM);
}

void Status_LED_Red(void)
{
    Status_LED_All_Off();
    Macro_Clear_Bit(PIN_STATUS_RED_PORT->ODR, PIN_STATUS_RED_NUM);
}

void Status_LED_Green(void)
{
    Status_LED_All_Off();
    Macro_Clear_Bit(PIN_STATUS_GRN_PORT->ODR, PIN_STATUS_GRN_NUM);
}

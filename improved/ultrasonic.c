#include "device_driver.h"
#include "pin_map.h"

void Ultrasonic_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, PIN_ULTRA_RCC_BIT);

    /* Trig : 출력, Push-pull, High speed */
    Macro_Write_Block(PIN_ULTRA_PORT->MODER, 0x3, 0x1, PIN_POS2(PIN_ULTRA_TRIG));
    Macro_Clear_Bit(PIN_ULTRA_PORT->OTYPER, PIN_ULTRA_TRIG);
    Macro_Write_Block(PIN_ULTRA_PORT->OSPEEDR, 0x3, 0x3, PIN_POS2(PIN_ULTRA_TRIG));

    /* Echo : 입력, Pull-down */
    Macro_Write_Block(PIN_ULTRA_PORT->MODER, 0x3, 0x0, PIN_POS2(PIN_ULTRA_ECHO));
    Macro_Write_Block(PIN_ULTRA_PORT->PUPDR, 0x3, 0x2, PIN_POS2(PIN_ULTRA_ECHO));

    Macro_Clear_Bit(PIN_ULTRA_PORT->ODR, PIN_ULTRA_TRIG);
}

/*
 * 거리 측정.
 *  반환 >= 0 : 측정된 거리(cm)
 *  반환 -1   : Echo 가 올라오지 않음 (센서 미연결 / 무응답)
 *  반환 -2   : Echo 가 내려오지 않음 (측정 범위 초과 / 배선 단선)
 * 음수는 conveyor_fsm 에서 STATE_ERROR 전이 조건으로 사용된다.
 */
int Ultrasonic_Get_Distance(void)
{
    volatile int timeout;
    int time = 0;

    Macro_Set_Bit(PIN_ULTRA_PORT->ODR, PIN_ULTRA_TRIG);
    for (volatile int i = 0; i < 5000; i++);
    Macro_Clear_Bit(PIN_ULTRA_PORT->ODR, PIN_ULTRA_TRIG);

    /* Echo 상승 대기 */
    timeout = 2000000;
    while (Macro_Check_Bit_Clear(PIN_ULTRA_PORT->IDR, PIN_ULTRA_ECHO)) {
        if (--timeout == 0) return -1;
    }

    /* Echo High 유지 시간 측정 */
    timeout = 2000000;
    while (Macro_Check_Bit_Set(PIN_ULTRA_PORT->IDR, PIN_ULTRA_ECHO)) {
        time++;
        for (volatile int i = 0; i < 15; i++);
        if (--timeout == 0) return -2;
    }

    return time / 100;
}

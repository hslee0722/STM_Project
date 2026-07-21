#include "device_driver.h"

static int Key_Raw_Pressed(void)
{
    return Macro_Check_Bit_Clear(GPIOB->IDR, 5);
}

void Key_Poll_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 1);
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10);
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 10);
}

int Key_Get_Pressed(void)
{
    static int stable = 0;
    static int count = 0;
    static int reported = 0;

    int raw = Key_Raw_Pressed();

    if (raw == stable) {
        count = 0;
    } else {
        count++;
        if (count >= 3) {
            stable = raw;
            count = 0;
        }
    }

    if (stable && !reported) {
        reported = 1;
        return 1;
    }
    if (!stable) {
        reported = 0;
    }
    return 0;
}

void Key_Wait_Key_Pressed(void)
{
    while (!Key_Raw_Pressed());
}

void Key_Wait_Key_Released(void)
{
    while (Key_Raw_Pressed());
}

void Key_ISR_Enable(int en)
{
    (void)en;
}

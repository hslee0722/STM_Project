#include "device_driver.h"

extern uint32_t Get_Tick(void);

#define KEY_DEBOUNCE_MS 20U

static int      key_stable    = 0;
static int      key_candidate = 0;
static uint32_t key_change_tick = 0;
static int      key_event     = 0;

static int Key_Raw_Pressed(void)
{
    return Macro_Check_Bit_Clear(GPIOB->IDR, 5);
}

void Key_Poll_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 1);
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x0, 10);
    Macro_Write_Block(GPIOB->PUPDR, 0x3, 0x1, 10);

    key_stable      = 0;
    key_candidate   = 0;
    key_change_tick = Get_Tick();
    key_event       = 0;
}

void Key_Update(void)
{
    int raw = Key_Raw_Pressed();

    if (raw != key_candidate) {
        key_candidate   = raw;
        key_change_tick = Get_Tick();
        return;
    }

    if (key_candidate != key_stable &&
        (Get_Tick() - key_change_tick) >= KEY_DEBOUNCE_MS) {
        key_stable = key_candidate;
        if (key_stable) {
            key_event = 1;
        }
    }
}

int Key_Get_Pressed(void)
{
    if (key_event) {
        key_event = 0;
        return 1;
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

#include "device_driver.h"
#include "pin_map.h"

extern uint32_t Get_Tick(void);

#define KEY_DEBOUNCE_MS 20U

static int      key_stable    = 0;
static int      key_candidate = 0;
static uint32_t key_change_tick = 0;
static int      key_event     = 0;

/* 버튼은 Pull-up + Active Low : 눌리면 IDR 비트가 0 */
static int Key_Raw_Pressed(void)
{
    return Macro_Check_Bit_Clear(PIN_BUTTON_PORT->IDR, PIN_BUTTON_NUM);
}

void Key_Poll_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, PIN_BUTTON_RCC_BIT);
    Macro_Write_Block(PIN_BUTTON_PORT->MODER, 0x3, 0x0, PIN_POS2(PIN_BUTTON_NUM)); /* Input  */
    Macro_Write_Block(PIN_BUTTON_PORT->PUPDR, 0x3, 0x1, PIN_POS2(PIN_BUTTON_NUM)); /* Pull-up*/

    key_stable      = 0;
    key_candidate   = 0;
    key_change_tick = Get_Tick();
    key_event       = 0;
}

/*
 * 메인 루프에서 매번 호출된다.
 * 원시 입력이 KEY_DEBOUNCE_MS 동안 흔들리지 않고 유지될 때만 상태를 확정하고,
 * 눌림 방향(상승 엣지)에서만 이벤트를 1회 발생시킨다.
 */
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

/* 이벤트를 1회 소비한다. FSM이 STATE_WAIT에서 호출 */
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

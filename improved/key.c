#include "device_driver.h"
#include "pin_map.h"

extern uint32_t Get_Tick(void);

#define KEY_DEBOUNCE_MS 20U


typedef enum {
    KEY_PH_DEBOUNCE_PRESS = 0, 
    KEY_PH_WAIT_RELEASE,       
    KEY_PH_DEBOUNCE_RELEASE    
} KeyPhase;

static volatile uint32_t key_tick    = 0;
static volatile int      key_pending = 0;  
static volatile int      key_event   = 0;  
static KeyPhase          key_phase   = KEY_PH_DEBOUNCE_PRESS;

/* Pull-up + Active Low : 눌리면 IDR 비트가 0 */
static int Key_Raw_Pressed(void)
{
    return Macro_Check_Bit_Clear(PIN_BUTTON_PORT->IDR, PIN_BUTTON_NUM);
}

void Key_Init(void)
{
    /* 1. GPIO : 입력 + 내부 풀업 */
    Macro_Set_Bit(RCC->AHB1ENR, PIN_BUTTON_RCC_BIT);
    Macro_Write_Block(PIN_BUTTON_PORT->MODER, 0x3, 0x0, PIN_POS2(PIN_BUTTON_NUM));
    Macro_Write_Block(PIN_BUTTON_PORT->PUPDR, 0x3, 0x1, PIN_POS2(PIN_BUTTON_NUM));

    /* 2. SYSCFG 클럭 (EXTI 라인 - 포트 매핑에 필요) */
    Macro_Set_Bit(RCC->APB2ENR, 14);

    /* 3. EXTI5 의 소스를 GPIOB 로 지정
     *    EXTICR[5/4] 의 (5%4)*4 = 4번 비트부터, 포트B = 0x1 */
    Macro_Write_Block(SYSCFG->EXTICR[PIN_BUTTON_NUM / 4], 0xF, 0x1,
                      (PIN_BUTTON_NUM % 4) * 4);

    /* 4. 하강 엣지 트리거 (풀업이므로 누르는 순간 High -> Low) */
    Macro_Set_Bit(EXTI->FTSR, PIN_BUTTON_NUM);
    Macro_Clear_Bit(EXTI->RTSR, PIN_BUTTON_NUM);

    /* 5. 대기 중이던 플래그를 지우고 마스크 해제 */
    EXTI->PR = (1u << PIN_BUTTON_NUM);          /* rc_w1 : 1을 써서 클리어 */
    Macro_Set_Bit(EXTI->IMR, PIN_BUTTON_NUM);

    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    key_tick    = Get_Tick();
    key_pending = 0;
    key_event   = 0;
    key_phase   = KEY_PH_DEBOUNCE_PRESS;
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & (1u << PIN_BUTTON_NUM)) {
        EXTI->PR = (1u << PIN_BUTTON_NUM);

        Macro_Clear_Bit(EXTI->IMR, PIN_BUTTON_NUM);

        key_tick    = Get_Tick();
        key_pending = 1;
    }
}

void Key_Update(void)
{
    uint32_t now;

    if (!key_pending) return;

    now = Get_Tick();

    switch (key_phase)
    {
        case KEY_PH_DEBOUNCE_PRESS:
            if (now - key_tick < KEY_DEBOUNCE_MS) break;
            /* 20ms 뒤에도 눌려 있으면 진짜 눌림, 아니면 노이즈로 폐기 */
            if (Key_Raw_Pressed()) key_event = 1;
            key_phase = KEY_PH_WAIT_RELEASE;
            break;

        case KEY_PH_WAIT_RELEASE:
            if (Key_Raw_Pressed()) break;
            key_tick  = now;
            key_phase = KEY_PH_DEBOUNCE_RELEASE;
            break;

        case KEY_PH_DEBOUNCE_RELEASE:
            if (now - key_tick < KEY_DEBOUNCE_MS) break;
            if (Key_Raw_Pressed()) {         
                key_phase = KEY_PH_WAIT_RELEASE;
                break;
            }
            /* 완전히 떨어졌다 -> 인터럽트 재무장 */
            key_pending = 0;
            key_phase   = KEY_PH_DEBOUNCE_PRESS;
            EXTI->PR    = (1u << PIN_BUTTON_NUM);
            Macro_Set_Bit(EXTI->IMR, PIN_BUTTON_NUM);
            break;

        default:
            key_phase = KEY_PH_DEBOUNCE_PRESS;
            break;
    }
}

/* 확정된 눌림을 1회 소비 */
int Key_Get_Pressed(void)
{
    if (key_event) {
        key_event = 0;
        return 1;
    }
    return 0;
}

/*
 * 쌓여 있던 눌림을 버린다.
 * STATE_WAIT 진입 직전에 호출해, 컨베이어가 이동하는 동안 눌린 버튼이
 * 도착하자마자 확인 처리로 소비되는 것을 막는다.
 */
void Key_Clear_Event(void)
{
    key_event = 0;
}

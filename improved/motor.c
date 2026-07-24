#include "device_driver.h"
#include "pin_map.h"

extern uint32_t Get_Tick(void);

volatile int s1_target = 0, s1_curr = 0; uint32_t s1_last = 0;
volatile int s2_target = 0, s2_curr = 0; uint32_t s2_last = 0;
volatile int servo_step = 0; uint32_t servo_last = 0;
volatile int dispense_servo_pending = 0;
volatile int supply_after_slot_pending = 0;
volatile int conveyor_start_pending = 0;

void Motor_Init(void)
{
    /* ── DC 모터 : TIM5_CH1(CW) / TIM5_CH2(CCW) ── */
    Macro_Set_Bit(RCC->AHB1ENR, PIN_DCMOTOR_RCC_BIT);
    Macro_Set_Bit(RCC->APB1ENR, 3);                    /* TIM5 */

    Macro_Write_Block(PIN_DCMOTOR_PORT->MODER, 0x3, 0x2, PIN_POS2(PIN_DCMOTOR_CW_NUM));
    Macro_Write_Block(PIN_DCMOTOR_PORT->MODER, 0x3, 0x2, PIN_POS2(PIN_DCMOTOR_CCW_NUM));
    Macro_Write_Block(PIN_DCMOTOR_PORT->AFR[PIN_AFR_IDX(PIN_DCMOTOR_CW_NUM)],
                      0xF, PIN_DCMOTOR_AF, PIN_AFR_POS(PIN_DCMOTOR_CW_NUM));
    Macro_Write_Block(PIN_DCMOTOR_PORT->AFR[PIN_AFR_IDX(PIN_DCMOTOR_CCW_NUM)],
                      0xF, PIN_DCMOTOR_AF, PIN_AFR_POS(PIN_DCMOTOR_CCW_NUM));
    Macro_Clear_Bit(PIN_DCMOTOR_PORT->OTYPER, PIN_DCMOTOR_CW_NUM);
    Macro_Clear_Bit(PIN_DCMOTOR_PORT->OTYPER, PIN_DCMOTOR_CCW_NUM);

    TIM5->PSC = 11;
    TIM5->ARR = 79999;
    TIM5->CCMR1 = (0x6 << 4) | (1 << 3) | (0x6 << 12) | (1 << 11);
    TIM5->CCER = (1 << 0) | (1 << 4);
    TIM5->CCR1 = 0;
    TIM5->CCR2 = 0;
    TIM5->EGR |= 1;
    TIM5->CR1 |= 1;

    /* ── 서보모터 : 배출구 개폐 (TIM3_CH1) ── */
    Macro_Set_Bit(RCC->AHB1ENR, PIN_SERVO_RCC_BIT);
    Macro_Set_Bit(RCC->APB1ENR, 1);                    /* TIM3 */

    Macro_Write_Block(PIN_SERVO_PORT->MODER, 0x3, 0x2, PIN_POS2(PIN_SERVO_NUM));
    Macro_Write_Block(PIN_SERVO_PORT->AFR[PIN_AFR_IDX(PIN_SERVO_NUM)],
                      0xF, PIN_SERVO_AF, PIN_AFR_POS(PIN_SERVO_NUM));

    TIM3->PSC = 84 - 1;
    TIM3->ARR = 20000 - 1;
    TIM3->CCMR1 |= (6 << 4);
    TIM3->CCER |= (1 << 0);
    TIM3->CR1 |= (1 << 0);
    TIM3->CCR1 = 0;

    /* ── 스텝모터1(약통 슬롯) / 스텝모터2(호퍼 공급) : 각 4상 출력 ── */
    Macro_Set_Bit(RCC->AHB1ENR, PIN_STEPPER1_RCC_BIT);
    Macro_Set_Bit(RCC->AHB1ENR, PIN_STEPPER2_RCC_BIT);

    /* 4핀 = MODER 8비트, 0x55 = 네 핀 모두 General purpose output */
    Macro_Write_Block(PIN_STEPPER1_PORT->MODER, 0xFF, 0x55, PIN_POS2(PIN_STEPPER1_SHIFT));
    Macro_Write_Block(PIN_STEPPER2_PORT->MODER, 0xFF, 0x55, PIN_POS2(PIN_STEPPER2_SHIFT));
}

void Stepper_Step(int step_num)
{
    unsigned int temp = PIN_STEPPER1_PORT->ODR;
    temp &= ~PIN_STEPPER1_MASK;
    switch (step_num % 4) {
        case 0: temp |= (0x09u << PIN_STEPPER1_SHIFT); break;
        case 1: temp |= (0x03u << PIN_STEPPER1_SHIFT); break;
        case 2: temp |= (0x06u << PIN_STEPPER1_SHIFT); break;
        case 3: temp |= (0x0Cu << PIN_STEPPER1_SHIFT); break;
    }
    PIN_STEPPER1_PORT->ODR = temp;
}

void Stepper2_Step(int step_num)
{
    unsigned int temp = PIN_STEPPER2_PORT->ODR;
    temp &= ~PIN_STEPPER2_MASK;
    int step_val = 0;
    switch (step_num % 4) {
        case 0: step_val = 0x09; break;
        case 1: step_val = 0x03; break;
        case 2: step_val = 0x06; break;
        case 3: step_val = 0x0C; break;
    }
    temp |= ((unsigned)step_val << PIN_STEPPER2_SHIFT);
    PIN_STEPPER2_PORT->ODR = temp;
}

void Rotate_Next_Slot_Async(void) { s1_target += 1141; }
/* 슬롯을 먼저 돌린 뒤, 회전이 끝나면 호퍼가 약을 떨어뜨린다 (동시 구동 방지) */
void Rotate_Then_Supply_Async(void) { s1_target += 1141; supply_after_slot_pending = 1; }
void Dispense_Rotate_Async(void)  { s1_target += 1141; dispense_servo_pending = 1; }
void Supply_Pill_Async(void)      { s2_target += 1141; }
void Stepper2_One_Day_Async(void) { s2_target += 2290; }
void Servo_Open_Close_Async(void) { if (servo_step == 0) { servo_step = 1; servo_last = Get_Tick(); } }

void Motor_Update_Task(void)
{
    uint32_t now = Get_Tick();

    /* ── 스텝모터1 : 메인 약통 슬롯 회전 ── */
    if (s1_curr < s1_target) {
        if (now - s1_last >= 4) { s1_last = now; Stepper_Step(s1_curr++); }
    } else {
        PIN_STEPPER1_PORT->ODR &= ~PIN_STEPPER1_MASK;   /* 대기 중 여자 전류 차단 */

        /* 슬롯 회전이 끝난 뒤에 이어서 실행할 동작들 */
        if (dispense_servo_pending) {
            dispense_servo_pending = 0;
            Servo_Open_Close_Async();
        }
        if (supply_after_slot_pending) {
            supply_after_slot_pending = 0;
            Supply_Pill_Async();
        }
    }

    /* ── 스텝모터2 : 상부 호퍼 약 공급 ── */
    if (s2_curr < s2_target) {
        if (now - s2_last >= 4) { s2_last = now; Stepper2_Step(s2_curr++); }
    } else {
        PIN_STEPPER2_PORT->ODR &= ~PIN_STEPPER2_MASK;   /* 대기 중 여자 전류 차단 */
    }

    switch (servo_step) {
        case 1: TIM3->CCR1 = 1400; if (now - servo_last >= 500)  { servo_step = 2; servo_last = now; conveyor_start_pending = 1; } break;
        case 2:                    if (now - servo_last >= 2000) { servo_step = 3; servo_last = now; } break;
        case 3: TIM3->CCR1 = 2000; if (now - servo_last >= 500)  { servo_step = 4; servo_last = now; } break;
        case 4: TIM3->CCR1 = 0; servo_step = 0; break;
    }
}

int Motor_All_Idle(void)
{
    return (s1_curr >= s1_target && s2_curr >= s2_target && servo_step == 0
            && !dispense_servo_pending && !supply_after_slot_pending);
}

#define MOTOR_STOP 0
#define MOTOR_CW   1
#define MOTOR_CCW -1
static unsigned int Motor_Percent = 37;
static int Motor_Dir = MOTOR_STOP;

void Motor_Set_Percent(unsigned int percent)
{
    if (percent < 20)  percent = 20;
    if (percent > 100) percent = 100;
    Motor_Percent = percent;
}

unsigned int Motor_Get_Percent(void) { return Motor_Percent; }
int Motor_Get_Dir(void) { return Motor_Dir; }

void Motor_Apply_Duty(void)
{
    unsigned int duty = (unsigned int)(((unsigned long)(TIM5->ARR + 1) * Motor_Percent) / 100);
    if (Motor_Dir == MOTOR_CW)       { TIM5->CCR2 = 0; TIM5->CCR1 = duty; }
    else if (Motor_Dir == MOTOR_CCW) { TIM5->CCR1 = 0; TIM5->CCR2 = duty; }
    else                             { TIM5->CCR1 = 0; TIM5->CCR2 = 0; }
}

void Stop(void) { TIM5->CCR1 = 0; TIM5->CCR2 = 0; Motor_Dir = MOTOR_STOP; }

void Move_CW(void)
{
    TIM5->CCR2 = 0;
    for (volatile int i = 0; i < 100; i++);
    Motor_Dir = MOTOR_CW;
    Motor_Apply_Duty();
}

void Move_CCW(void)
{
    TIM5->CCR1 = 0;
    for (volatile int i = 0; i < 100; i++);
    Motor_Dir = MOTOR_CCW;
    Motor_Apply_Duty();
}

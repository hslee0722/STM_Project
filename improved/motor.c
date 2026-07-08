#include "device_driver.h"

extern uint32_t Get_Tick(void);

volatile int s1_target = 0, s1_curr = 0; uint32_t s1_last = 0;
volatile int s2_target = 0, s2_curr = 0; uint32_t s2_last = 0;
volatile int servo_step = 0; uint32_t servo_last = 0;

void Motor_Init(void)
{
    Macro_Set_Bit(RCC->APB1ENR, 3);
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xA, 0);
    Macro_Write_Block(GPIOA->AFR[0], 0xFF, 0x22, 0);
    Macro_Clear_Bit(GPIOA->OTYPER, 0);
    Macro_Clear_Bit(GPIOA->OTYPER, 1);
    TIM5->PSC = 11;
    TIM5->ARR = 79999;
    TIM5->CCMR1 = (0x6 << 4) | (1 << 3) | (0x6 << 12) | (1 << 11);
    TIM5->CCER = (1 << 0) | (1 << 4);
    TIM5->CCR1 = 0;
    TIM5->CCR2 = 0;
    TIM5->EGR |= 1;
    TIM5->CR1 |= 1;

    Macro_Set_Bit(RCC->APB1ENR, 1);
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x2, 12);
    Macro_Write_Block(GPIOA->AFR[0], 0xF, 0x2, 24);
    TIM3->PSC = 84 - 1;
    TIM3->ARR = 20000 - 1;
    TIM3->CCMR1 |= (6 << 4);
    TIM3->CCER |= (1 << 0);
    TIM3->CR1 |= (1 << 0);
    TIM3->CCR1 = 0;

    Macro_Set_Bit(RCC->AHB1ENR, 2);
    GPIOC->MODER &= ~(0xFF << 0);  GPIOC->MODER |= (0x55 << 0);
    GPIOC->MODER &= ~(0xFF << 12); GPIOC->MODER |= (0x55 << 12);
}

void Stepper_Step(int step_num)
{
    unsigned int temp = GPIOC->ODR;
    temp &= ~(0xF << 0);
    switch (step_num % 4) {
        case 0: temp |= 0x09; break;
        case 1: temp |= 0x03; break;
        case 2: temp |= 0x06; break;
        case 3: temp |= 0x0C; break;
    }
    GPIOC->ODR = temp;
}

void Stepper2_Step(int step_num)
{
    unsigned int temp = GPIOC->ODR;
    temp &= ~(0xF << 6);
    int step_val = 0;
    switch (step_num % 4) {
        case 0: step_val = 0x09; break;
        case 1: step_val = 0x03; break;
        case 2: step_val = 0x06; break;
        case 3: step_val = 0x0C; break;
    }
    temp |= (step_val << 6);
    GPIOC->ODR = temp;
}

void Rotate_Next_Slot_Async(void) { s1_target += 1141; }
void Supply_Pill_Async(void)      { s2_target += 1141; }
void Stepper2_One_Day_Async(void) { s2_target += 2290; }
void Servo_Open_Close_Async(void) { if (servo_step == 0) { servo_step = 1; servo_last = Get_Tick(); } }

void Motor_Update_Task(void)
{
    uint32_t now = Get_Tick();

    if (s1_curr < s1_target) {
        if (now - s1_last >= 4) { s1_last = now; Stepper_Step(s1_curr++); }
    } else { GPIOC->ODR &= ~(0xF << 0); }

    if (s2_curr < s2_target) {
        if (now - s2_last >= 4) { s2_last = now; Stepper2_Step(s2_curr++); }
    } else { GPIOC->ODR &= ~(0xF << 6); }

    switch (servo_step) {
        case 1: TIM3->CCR1 = 1400; if (now - servo_last >= 500)  { servo_step = 2; servo_last = now; } break;
        case 2:                    if (now - servo_last >= 2000) { servo_step = 3; servo_last = now; } break;
        case 3: TIM3->CCR1 = 2000; if (now - servo_last >= 500)  { servo_step = 4; servo_last = now; } break;
        case 4: TIM3->CCR1 = 0; servo_step = 0; break;
    }
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

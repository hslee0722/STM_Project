#include "device_driver.h"

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

void Tick_Init(void)
{
    SysTick_Config(96000000 / 1000);
}

uint32_t Get_Tick(void)
{
    return ms_ticks;
}

void Delay_ms(unsigned int msec)
{
    if (msec == 0) return;
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < msec);
}

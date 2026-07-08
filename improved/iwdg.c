#include "device_driver.h"
#include "iwdg.h"

void IWDG_Init(void)
{
    IWDG->KR  = 0x5555;
    IWDG->PR  = 0x4;
    IWDG->RLR = 1000;
    IWDG->KR  = 0xCCCC;
}

void IWDG_Refresh(void)
{
    IWDG->KR = 0xAAAA;
}

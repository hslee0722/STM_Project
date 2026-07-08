#include "device_driver.h"
#include "rtc_calibration.h"

void RTC_SmoothCalib_Set(int calp, int calm)
{
    if (calm < 0)   calm = 0;
    if (calm > 511) calm = 511;

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    while (RTC->ISR & (1 << 16));

    RTC->CALR = ((calp ? 1 : 0) << 15) | (calm & 0x1FF);

    RTC->WPR = 0xFF;
}

void RTC_Calibration_From_Daily_Error(float daily_error_sec)
{
    if (daily_error_sec > 0.0f) {
        int calm = (int)(daily_error_sec * 6.0f);
        RTC_SmoothCalib_Set(0, calm);
    } else if (daily_error_sec < 0.0f) {
        RTC_SmoothCalib_Set(1, 0);
    } else {
        RTC_SmoothCalib_Set(0, 0);
    }
}

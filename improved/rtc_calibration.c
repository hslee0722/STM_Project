#include "device_driver.h"
#include "rtc_calibration.h"

/*
 * RTC Smooth Calibration (RTC_CALR)
 *
 *   CALM[8:0] : 2^20 RTCCLK 주기마다 CALM 개의 펄스를 "제거" -> 시계를 늦춘다
 *   CALP      : 같은 주기마다 512 펄스를 "추가"              -> 시계를 빠르게 한다
 *
 *   1 CALM 단위  = 1 / 2^20        = 0.9537 ppm
 *   하루 1초 오차 = 1 / 86400 * 1e6 = 11.574 ppm
 *              -> 하루 1초당 11.574 / 0.9537 = 12.135 CALM 단위
 *
 *   보정 범위 : 느림 최대 -42.2 s/day (CALP=1) ~ 빠름 최대 +42.1 s/day (CALM=511)
 */
#define CALM_PER_SEC_PER_DAY   12.135f
#define CALM_MAX               511
#define CALP_UNITS             512      /* CALP=1 이 더해주는 펄스 수 */

/* CALR 는 직전 보정이 반영될 때까지(RECALPF=0) 쓰면 안 된다 */
static int RTC_Wait_Recalp_Clear(void)
{
    volatile unsigned int timeout = 500000;

    while (RTC->ISR & (1 << 16)) {
        if (--timeout == 0) return -1;
    }
    return 0;
}

int RTC_SmoothCalib_Set(int calp, int calm)
{
    if (calm < 0)        calm = 0;
    if (calm > CALM_MAX) calm = CALM_MAX;

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    if (RTC_Wait_Recalp_Clear() < 0) {
        RTC->WPR = 0xFF;
        return -1;
    }

    RTC->CALR = ((calp ? 1u : 0u) << 15) | ((unsigned int)calm & 0x1FF);

    RTC->WPR = 0xFF;
    return 0;
}

int RTC_Calibration_From_Daily_Error(float daily_error_sec)
{
    int units;

    if (daily_error_sec > 0.0f) {
        /* 시계가 빠름 -> 펄스를 빼서 늦춘다 */
        units = (int)(daily_error_sec * CALM_PER_SEC_PER_DAY + 0.5f);
        if (units > CALM_MAX) units = CALM_MAX;
        return RTC_SmoothCalib_Set(0, units);
    }

    if (daily_error_sec < 0.0f) {
        /* 시계가 느림 -> CALP 로 512 펄스를 더한 뒤 초과분만 CALM 으로 깎는다 */
        units = (int)(-daily_error_sec * CALM_PER_SEC_PER_DAY + 0.5f);
        if (units > CALP_UNITS) units = CALP_UNITS;
        return RTC_SmoothCalib_Set(1, CALP_UNITS - units);
    }

    return RTC_SmoothCalib_Set(0, 0);
}

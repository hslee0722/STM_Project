#include "device_driver.h"
#include <stdio.h>

/*
 * rtc_calibration_example.c
 * ------------------------------------------------------------
 * RTC 오차 보정 예시 파일입니다.
 *
 * 이 파일은 바로 컴파일에 넣기보다는 참고용으로 보는 것을 권장합니다.
 * STM32F411의 RTC_CALR bit 정의와 보정 범위는 reference manual 기준으로
 * 한 번 더 확인해야 합니다.
 */

/*
 * RTC_SmoothCalib_Set()
 * ------------------------------------------------------------
 * calp:
 *   0 = pulse 추가 없음
 *   1 = pulse 추가
 *
 * calm:
 *   0~511 범위
 *   보정 pulse를 제거하는 값
 *
 * 개념:
 *   RTC가 빠르면 calm 값을 증가시켜 clock pulse를 일부 제거
 *   RTC가 느리면 calp 사용을 검토
 */
void RTC_SmoothCalib_Set(int calp, int calm)
{
    if (calm < 0) calm = 0;
    if (calm > 511) calm = 511;

    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /*
     * RECALPF bit가 0이 될 때까지 기다려야 합니다.
     * 일부 헤더에서는 RTC_ISR_RECALPF 이름으로 정의되어 있을 수 있습니다.
     */
    while (RTC->ISR & (1 << 16));

    RTC->CALR = ((calp ? 1 : 0) << 15) | (calm & 0x1FF);

    RTC->WPR = 0xFF;

    printf("[RTC] Smooth calibration set. CALP=%d, CALM=%d\r\n", calp, calm);
}

/*
 * RTC_Calibration_From_Daily_Error()
 * ------------------------------------------------------------
 * daily_error_sec:
 *   하루 기준 RTC 오차입니다.
 *
 * 예:
 *   +2.0초 : RTC가 실제보다 하루에 2초 빠름
 *   -2.0초 : RTC가 실제보다 하루에 2초 느림
 *
 * 실제 CALM 환산은 보드/클럭/캘리브레이션 윈도우 설정에 따라 달라질 수 있어
 * 여기서는 포트폴리오용 개념 함수로 둡니다.
 */
void RTC_Calibration_From_Daily_Error(float daily_error_sec)
{
    if (daily_error_sec > 0.0f) {
        /*
         * RTC가 빠름 → pulse 제거
         * calm 값을 경험적으로 증가
         */
        int calm = (int)(daily_error_sec * 6.0f);
        RTC_SmoothCalib_Set(0, calm);
    }
    else if (daily_error_sec < 0.0f) {
        /*
         * RTC가 느림 → pulse 추가 방향
         * 실제로는 CALP 사용 가능 여부와 범위를 확인해야 함
         */
        RTC_SmoothCalib_Set(1, 0);
    }
    else {
        RTC_SmoothCalib_Set(0, 0);
    }
}

#ifndef RTC_CALIBRATION_H
#define RTC_CALIBRATION_H

#ifndef RTC_CALIB_DAILY_ERROR_SEC
#define RTC_CALIB_DAILY_ERROR_SEC   1.3f
#endif

int RTC_SmoothCalib_Set(int calp, int calm);
int RTC_Calibration_From_Daily_Error(float daily_error_sec);

#endif

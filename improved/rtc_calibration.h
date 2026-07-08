#ifndef RTC_CALIBRATION_H
#define RTC_CALIBRATION_H

void RTC_SmoothCalib_Set(int calp, int calm);
void RTC_Calibration_From_Daily_Error(float daily_error_sec);

#endif

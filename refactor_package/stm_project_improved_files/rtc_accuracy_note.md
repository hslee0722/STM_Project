# RTC 시간 오차 줄이는 방법

STM32 RTC 시간 오차는 크게 3가지 원인에서 발생합니다.

1. LSE 32.768kHz 크리스탈 자체 오차
2. 크리스탈 주변 부하 커패시터 값 오차
3. 온도 변화와 보드 레이아웃 영향

## 1. LSE 사용 확인

현재 프로젝트는 `alarm.c`에서 LSE를 켜고 RTC clock source로 선택합니다.

```c
RCC->BDCR |= (1 << 0);     // LSEON
while(!(RCC->BDCR & (1 << 1)));
RCC->BDCR |= (0x1 << 8);   // RTCSEL = LSE
```

이 방향은 맞습니다. HSI/LSI보다 LSE가 RTC 오차가 훨씬 작습니다.

## 2. 오차 측정 방법

예를 들어 실제 시간보다 하루에 2초 빠르다면:

```text
1일 오차 = +2초
30일 오차 = +60초
```

이 경우 RTC가 빠른 것이므로 RTC tick을 조금 늦춰야 합니다.

반대로 하루에 2초 느리다면 tick을 보정해서 빠르게 해야 합니다.

## 3. RTC Smooth Calibration 사용

STM32F4 RTC에는 `RTC_CALR` 레지스터를 이용한 smooth calibration 기능이 있습니다.

개념:

```text
RTC가 빠르다  → 일부 clock pulse를 빼서 느리게 보정
RTC가 느리다  → CALP bit로 보정 pulse를 추가
```

아래 코드는 예시입니다. 보드에서 실제 오차를 측정한 후 값을 조정해야 합니다.

```c
void RTC_SmoothCalib_Set(int calp, int calm)
{
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    while (RTC->ISR & (1 << 16));

    RTC->CALR = ((calp ? 1 : 0) << 15) | (calm & 0x1FF);

    RTC->WPR = 0xFF;
}
```

주의:
- `calm` 값은 보정 pulse를 빼는 양입니다.
- `calp`는 보정 pulse를 추가하는 옵션입니다.
- 정확한 bit 정의는 사용하는 STM32F411 reference manual 기준으로 확인해야 합니다.

## 4. 현실적인 프로젝트 개선안

포트폴리오 수준에서는 아래처럼 구현했다고 정리하면 좋습니다.

```text
RTC는 LSE 32.768kHz 외부 크리스탈을 사용해 기본 정확도를 확보했다.
추가 개선으로 일정 기간 실제 시간과 RTC 시간을 비교해 일 오차를 계산하고,
RTC smooth calibration 값을 조정하는 보정 루틴을 설계했다.
```

## 5. 더 좋은 하드웨어 개선

- 20ppm 이하 LSE 크리스탈 사용
- 크리스탈 데이터시트에 맞는 load capacitor 선정
- RTC용 외부 모듈 DS3231 사용
- BLE 앱과 주기적으로 시간 동기화
- 전원 OFF 후에도 유지하려면 VBAT 백업 배터리 사용

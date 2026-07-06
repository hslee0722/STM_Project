# STM_Project 개선안 적용 가이드

이 폴더는 기존 프로젝트를 바로 덮어쓰기 위한 완성본이라기보다,
현재 코드 구조를 개선하기 위한 적용용 파일 묶음입니다.

## 추가된 개선점

1. `conveyor_fsm.c/h`
   - main.c에 있던 컨베이어 상태 머신 분리
   - IDLE/FORWARD/WAIT/BACKWARD/FINISHED/ERROR 구조 명확화
   - 초음파 센서 에러(-1, -2) 발생 시 모터 정지

2. `command_parser.c/h`
   - UART 문자열 명령 파싱 분리
   - T, A, L, f, servo, step, store 명령을 구조체로 변환

3. `nvic_config.c/h`
   - RTC Alarm, UART1, UART2, EXTI 우선순위 설정
   - 숫자 IRQ 대신 CMSIS IRQn 이름 사용 권장

4. `pin_map.h`
   - 핀 정보를 한 곳에 정리

5. `rtc_accuracy_note.md`
   - RTC 오차 원인과 줄이는 방법 정리
   - LSE, smooth calibration, BLE 주기 동기화, DS3231 대안 포함

6. `rtc_calibration_example.c`
   - RTC smooth calibration 개념 코드 추가

7. `main_refactor_example.c`
   - 기존 main.c를 어떻게 줄일 수 있는지 보여주는 예시

## 적용 순서 추천

### 1단계: 백업

기존 `Final_Project` 폴더를 먼저 백업하세요.

```text
Final_Project_backup
```

### 2단계: 새 파일 추가

아래 파일을 `Final_Project/`에 추가합니다.

```text
app_state.h
pin_map.h
conveyor_fsm.c
conveyor_fsm.h
command_parser.c
command_parser.h
nvic_config.c
nvic_config.h
```

Makefile은 `*.c`를 자동으로 컴파일하므로 같은 폴더에 넣으면 컴파일 대상에 포함됩니다.

### 3단계: main.c 정리

바로 `main_refactor_example.c`로 교체하지 말고,
기존 `main.c`와 비교하면서 다음 순서로 옮기는 것을 추천합니다.

1. 상태 정의 제거 → `app_state.h` 사용
2. 컨베이어 switch-case 제거 → `Conveyor_Update(dist)` 사용
3. UART2 명령 처리 제거 → `Command_Parse()`, `Command_Execute()` 사용
4. `Sys_Init()`에 `NVIC_Config()` 추가

### 4단계: 중복 함수명/전역변수 확인

현재 기존 코드에는 아래 전역변수가 있습니다.

```c
volatile int pill_alarm_flag;
volatile int auto_load_flag;
char auto_load_data[8];
```

새 파일에서도 같은 전역변수를 선언하면 중복 정의 에러가 날 수 있습니다.

원칙:
- `.h` 파일에는 `extern` 선언만
- 실제 정의는 `main.c` 한 곳에서만

### 5단계: RTC 보정은 바로 넣지 말고 측정 후 적용

RTC smooth calibration은 실제 보드에서 오차를 측정한 뒤 적용해야 합니다.
무작정 값을 넣으면 오히려 시간이 더 틀어질 수 있습니다.

## 면접에서 설명하기 좋은 문장

기존에는 main.c에 UART 처리, RTC 처리, 컨베이어 상태 머신, 자동 적재 로직이 섞여 있었습니다.
개선안에서는 명령 파서, 컨베이어 FSM, NVIC 설정, 핀맵을 모듈화하여 유지보수성을 높였습니다.
또한 초음파 센서 에러 시 모터를 정지하는 안전 로직과 RTC 오차 보정을 위한 smooth calibration 개선 방향을 추가했습니다.

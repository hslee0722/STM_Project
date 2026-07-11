# 🔄 STM32 Pill Dispenser — Improved Firmware

> [`Final_Project`](../Final_Project)의 초기 통합형 펌웨어를 기준으로 병목과 유지보수 문제를 분석하고, **모듈형·논블로킹 구조**로 리팩터링한 개선 버전입니다.

---

## 왜 리팩터링했는가

기존 [`Final_Project/main.c`](../Final_Project/main.c)는 UART 파싱, RTC 폴링, 컨베이어 FSM, 모터 제어, LCD 출력이 **하나의 `while(1)` 루프**에 모두 섞여 있었습니다. 문제는:

- `TIM2_Delay()` 등 블로킹 딜레이 함수가 실행되는 동안 다른 입력(UART 명령, 센서 갱신)에 즉시 대응할 수 없었음
- 명령 처리가 `if / else if` 체인으로 되어 있어 명령이 늘어날수록 `main.c`가 비대해짐
- 센서 이상이나 루프 정지 상황에 대한 방어 로직이 없었음
- 핀 배치, 인터럽트 우선순위 같은 설정이 여러 파일에 흩어져 있어 하드웨어 변경 시 추적이 어려웠음

이 폴더는 위 문제들을 **기능 단위 모듈 분리 + Tick 기반 비동기 처리**로 해결한 버전입니다.

---

## 🔄 개선 전·후 비교

| 비교 항목 | 기존 · `Final_Project` | 개선 · `improved` |
|---|---|---|
| 프로그램 구조 | `main.c`에서 UART·RTC·FSM·모터·LCD를 함께 처리 | FSM·명령 파서·모터·시스템 설정을 독립 모듈로 분리 |
| 모터 제어 | Delay와 반복문 중심의 블로킹 동작 | `Get_Tick()` 기반 비동기 태스크 (`Motor_Update_Task`) |
| 명령 처리 | 메인 루프의 긴 `if / else if` 체인 | `ParsedCommand` 구조체로 파싱 후 형식 검증 → 실행 분리 |
| 오류 대응 | 센서 오류 전용 상태 없음 | `STATE_ERROR`에서 모터·부저·LED 안전 정지 |
| 시스템 안정성 | 기능별 설정이 여러 파일에 분산 | IWDG·NVIC 우선순위·RTC 보정·핀맵을 명시적으로 관리 |

---

## 🗂️ 모듈 구성

| 파일 | 역할 |
|---|---|
| `main.c` | 메인 루프 — 각 태스크를 순서대로 호출하는 경량 스케줄러 |
| `app_state.h` | 전역 상태 타입(`ConveyorState`)과 공유 플래그/버퍼 선언 |
| `conveyor_fsm.c` / `.h` | 컨베이어 상태 머신 (`STATE_IDLE ~ STATE_ERROR`) |
| `command_parser.c` / `.h` | UART 문자열 → `ParsedCommand` 파싱, 검증 후 `Command_Execute()`로 실행 |
| `motor.c` | DC/스텝/서보 모터 — Tick 기반 비동기 태스크(`*_Async`) + `Motor_Update_Task()` |
| `uart.c` | UART1(BLE)/UART2(디버그) 송수신 |
| `rtc_calibration.c` / `.h` | RTC Smooth Calibration 계산 (`RTC_SmoothCalib_Set`) |
| `nvic_config.c` / `.h` | 인터럽트 우선순위 명시적 설정 (`NVIC_Config`) |
| `iwdg.c` / `.h` | 독립 워치독 타이머 초기화/리프레시 |
| `pin_map.h` | 핀 번호, LCD I2C 주소 등 하드웨어 매핑 중앙 관리 |
| `systick.c`, `exception.c` | 시스템 틱, 예외 핸들러 |

---

## ⚙️ 주요 개선 사항

### 1. 메인 루프를 가벼운 스케줄러로 변경

**Before** — 센서 갱신, 화면 표시, 명령 파싱, 모터 동작이 한 루프에 뒤섞여 있어 지연 함수 실행 중 다른 입력에 즉시 대응하기 어려움

```c
while (1) {
    Process_UART_Input();
    dist = Ultrasonic_Get_Distance();
    switch (current_state) { /* 컨베이어 상태 전이와 모터 제어 */ }
    LCD2_Show_State(current_state, dist);
    Delay_ms(100);
    if (uart2_rx_exist) { /* 시간·알람·모터 명령 파싱 */ }
}
```

**After** — 메인 루프는 각 태스크를 호출하는 역할에 집중하고, 실제 동작은 함수/모듈 내부로 분리

```c
while (1) {
    IWDG_Refresh();
    Motor_Update_Task();

    Handle_Uart1_Command();
    Handle_Uart2_Command();
    Update_Clock_Display();

    if (pill_alarm_flag == 1)
        Handle_Pill_Alarm();

    if (Get_Tick() - last_sensor_time >= 100) {
        last_sensor_time = Get_Tick();
        dist = Ultrasonic_Get_Distance();
        Conveyor_Update(dist);
        LCD2_Show_State(Conveyor_Get_State(), dist);
    }
}
```

### 2. Tick 기반 논블로킹 모터 제어 ([`motor.c`](./motor.c))

`Rotate_Next_Slot_Async()`, `Supply_Pill_Async()`, `Servo_Open_Close_Async()`는 목표 스텝 수만 등록하고, `Motor_Update_Task()`가 매 루프마다 1ms Tick을 기준으로 조금씩 진행시킵니다.

```c
void Rotate_Next_Slot_Async(void) { s1_target += 1141; }
void Supply_Pill_Async(void)      { s2_target += 1141; }

void Motor_Update_Task(void)
{
    uint32_t now = Get_Tick();
    if (s1_curr < s1_target) {
        if (now - s1_last >= 4) { s1_last = now; Stepper_Step(s1_curr++); }
    } else {
        GPIOC->ODR &= ~(0xF << 0);   // 대기 중 전류 차단(발열 방지)
    }
    // s2, servo도 동일한 패턴으로 처리
}
```

모터가 움직이는 동안에도 UART 수신, 센서 확인, 워치독 갱신이 계속 진행됩니다.

### 3. 명령 파싱과 실행 분리 ([`command_parser.c`](./command_parser.c))

수신 문자열을 먼저 `ParsedCommand` 구조체로 검증한 뒤, 유효한 경우에만 `Command_Execute()`로 넘깁니다.

```c
if (rx[0] == 'L' || rx[0] == 'l') {
    if (strlen(rx) >= 8 && is_week_pattern(rx + 1)) {
        strncpy(cmd.week_data, rx + 1, 7);
        cmd.type = CMD_AUTO_LOAD;
    } else {
        cmd.type = CMD_INVALID;   // 잘못된 입력은 실행부로 전달되지 않음
    }
}
```

`Auto-Load`(`L1010100`) 명령은 7자리가 모두 `0`/`1`인지 `is_week_pattern()`으로 검증하며, 잘못된 명령에는 `Uart1_Printf("ERR\n")`으로 앱에 즉시 피드백합니다.

### 4. 고장 시 안전 상태로 전이 ([`conveyor_fsm.c`](./conveyor_fsm.c))

초음파 거리 값이 비정상(`dist < 0`)이면 모터·부저·LED를 안전하게 정지하고 `STATE_ERROR`로 전이합니다.

```c
void Conveyor_Update(int dist)
{
    if (dist < 0) {
        Stop();
        Buzzer_Off();
        Status_LED_All_Off();
        current_state = STATE_ERROR;
        return;
    }
    switch (current_state) { /* 기존 FSM 로직 */ }
}
```

### 5. 시스템 보호 및 설정 중앙화

- **워치독**: `IWDG_Init()` / `IWDG_Refresh()` — 메인 루프가 멈추면 MCU가 자동 복구
- **인터럽트 우선순위**: `NVIC_Config()`에서 RTC Alarm → USART1 → USART2 → 외부 버튼 순으로 명시적 설정
- **RTC 보정**: `rtc_calibration.c`에서 일일 오차(초)를 기반으로 `CALR` 레지스터 보정값 계산
- **핀맵 중앙화**: `pin_map.h`에서 버튼·부저·초음파·스텝모터·상태 LED 핀을 한 곳에서 관리

```c
void NVIC_Config(void)
{
    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(3, 0, 0));
    NVIC_SetPriority(USART1_IRQn,    NVIC_EncodePriority(3, 1, 0));
    NVIC_SetPriority(USART2_IRQn,    NVIC_EncodePriority(3, 2, 0));
    NVIC_SetPriority(EXTI15_10_IRQn, NVIC_EncodePriority(3, 3, 0));
}
```

---

## 개선의 의미

- **응답성**: 모터 동작 중에도 통신·센서·화면 갱신이 계속 진행됨
- **유지보수성**: 기능별 파일 경계가 명확해 수정 시 영향 범위가 축소됨
- **안전성**: 입력 오류, 센서 이상, 루프 정지에 대한 방어 로직 추가 (`CMD_INVALID`, `STATE_ERROR`, IWDG)
- **확장성**: 새로운 명령이나 상태를 독립 모듈에 추가할 수 있는 구조 확보

---

## 빌드

`Final_Project`와 동일한 레지스터 직접 제어 방식이며, 별도의 HAL 의존성은 없습니다. `device_driver.h`를 통해 상위 드라이버 함수(LCD, UART, RTC 등)를 공유합니다.

---

*원본 통합형 구현은 [`../Final_Project`](../Final_Project)와 [루트 README](../README.md)를 참고하세요.*

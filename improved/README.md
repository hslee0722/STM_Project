# 🔄 STM32 Pill Dispenser — Improved Firmware

> [`Final_Project`](../Final_Project)의 초기 통합형 펌웨어를 기준으로 병목과 유지보수 문제를 분석하고, **모듈형·논블로킹 구조**로 리팩터링한 개선 버전입니다.

---

## 왜 리팩터링했는가

기존 [`Final_Project/main.c`](../Final_Project/main.c)는 383줄 안에 UART 파싱, RTC 폴링, 컨베이어 FSM, 모터 제어, LCD 출력이 **하나의 `while(1)` 루프**에 모두 섞여 있었습니다. 문제는:

- `TIM2_Delay()`, `for(volatile int i=0;i<0x100000;i++)` 같은 블로킹 지연이 실행되는 동안 UART 명령·센서 갱신에 전혀 대응할 수 없었음
- 버튼 처리도 `Key_Wait_Key_Released()`로 **눌린 버튼이 떨어질 때까지 루프 전체가 멈춤**
- 명령 처리가 `if / else if` 체인으로 되어 있어 명령이 늘어날수록 `main.c`가 비대해짐
- 센서 이상이나 루프 정지 상황에 대한 방어 로직이 없었음
- `T`/`A` 명령이 인자를 파싱하지 않고 `0:0:0`을 하드코딩하고 있었음

이 폴더는 위 문제들을 **기능 단위 모듈 분리 + Tick 기반 비동기 처리**로 해결한 버전입니다.

---

## 🔄 개선 전·후 비교

| 비교 항목 | 기존 · `Final_Project` | 개선 · `improved` |
|---|---|---|
| 프로그램 구조 | `main.c` 383줄에서 UART·RTC·FSM·모터·LCD를 함께 처리 | FSM·명령 파서·모터·시스템 설정을 독립 모듈로 분리 (`main.c` 177줄) |
| 모터 제어 | `TIM2_Delay()` / busy-wait 기반 블로킹 동작 | `Get_Tick()` 기반 비동기 태스크 (`*_Async` + `Motor_Update_Task`) |
| 버튼 입력 | `Key_Wait_Key_Released()` — 루프 정지 | 20ms 디바운스 폴링 (`Key_Update` / `Key_Get_Pressed`) |
| 명령 처리 | 메인 루프의 긴 `if / else if` 체인 | `ParsedCommand` 구조체로 파싱·검증 → `Command_Execute()`로 실행 분리 |
| 시각/알람 설정 | `T`, `A` 명령이 `0:0:0` 하드코딩 | `T08:30:00` 형태로 인자 파싱, 실패 시 `CMD_INVALID` |
| BLE 수신 | UART2만 인터럽트, UART1은 폴링 | UART1 전용 ISR + 64B 라인 버퍼 (개행 기준 완료 판정) |
| 오류 대응 | 센서 오류 전용 상태 없음 | `STATE_ERROR`에서 모터·부저·LED 안전 정지 |
| 루프 정지 대응 | 없음 | IWDG(약 2초) 워치독으로 자동 리셋 복구 |

---

## 🗂️ 모듈 구성

| 파일 | 역할 |
|---|---|
| `main.c` | 메인 루프 — 각 태스크를 순서대로 호출하는 경량 스케줄러 |
| `app_state.h` | 전역 상태 타입(`ConveyorState`)과 공유 플래그/버퍼 선언 |
| `conveyor_fsm.c` / `.h` | 컨베이어 상태 머신 (`STATE_IDLE` ~ `STATE_ERROR`) |
| `command_parser.c` / `.h` | UART 문자열 → `ParsedCommand` 파싱, 검증 후 `Command_Execute()`로 실행 |
| `motor.c` | DC/스텝/서보 — Tick 기반 비동기 태스크(`*_Async`) + `Motor_Update_Task()` |
| `key.c` | 버튼 디바운스 폴링, 엣지 이벤트 큐잉 |
| `uart.c` | UART1(BLE, ISR 라인 버퍼) / UART2(디버그) 송수신 |
| `iwdg.c` / `.h` | 독립 워치독 타이머 초기화·리프레시 |
| `nvic_config.c` / `.h` | 인터럽트 우선순위 명시적 설정 (`NVIC_Config`) |
| `rtc_calibration.c` / `.h` | RTC Smooth Calibration 계산 API *(정의만, 아직 미호출 — 아래 참고)* |
| `pin_map.h` | 핀 번호·LCD I2C 주소 정의 *(아직 각 모듈에 적용 전 — 아래 참고)* |
| `systick.c` | 1ms SysTick — `Get_Tick()` 기준 클럭 |

---

## ⚙️ 주요 개선 사항

### 1. 메인 루프를 가벼운 스케줄러로 변경

**Before** — 센서 갱신, 화면 표시, 명령 파싱, 모터 동작이 한 루프에 뒤섞여 있고 `Delay_ms(100)`으로 루프 전체가 멈춤

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

**After** — 루프는 태스크 호출만 담당하고, 주기 작업은 Tick 비교로 처리해 블로킹 없이 진행

```c
while (1) {
    IWDG_Refresh();
    Key_Update();
    Motor_Update_Task();

    Handle_Uart1_Command();
    Handle_Uart2_Command();

    if (auto_load_flag == 1) {
        Auto_Load_Sequence(auto_load_data);
        auto_load_flag = 0;
        Conveyor_Init_State();
    }

    Update_Clock_Display();

    if (pill_alarm_flag == 1) Handle_Pill_Alarm();

    if (conveyor_start_pending == 1) {
        conveyor_start_pending = 0;
        Conveyor_Start();
    }

    if (Get_Tick() - last_sensor_time >= 100) {
        last_sensor_time = Get_Tick();
        dist = Ultrasonic_Get_Distance();
        Conveyor_Update(dist);
        LCD2_Show_State(Conveyor_Get_State(), dist);
    }
}
```

`Update_Clock_Display()`도 RTC 초 값이 바뀔 때만 LCD를 갱신해 불필요한 I2C 트랜잭션을 제거했습니다.

### 2. Tick 기반 논블로킹 모터 제어 ([`motor.c`](./motor.c))

`*_Async()` 함수는 **목표 스텝 수만 누적**하고 즉시 반환합니다. 실제 구동은 `Motor_Update_Task()`가 1ms Tick 기준으로 나눠서 진행합니다.

```c
void Rotate_Next_Slot_Async(void) { s1_target += 1141; }   // 약통 1칸 (s1)
void Supply_Pill_Async(void)      { s2_target += 1141; }   // 호퍼 공급 (s2)
void Stepper2_One_Day_Async(void) { s2_target += 2290; }

void Motor_Update_Task(void)
{
    uint32_t now = Get_Tick();
    if (s1_curr < s1_target) {
        if (now - s1_last >= 4) { s1_last = now; Stepper_Step(s1_curr++); }
    } else {
        GPIOC->ODR &= ~(0xF << 0);   // 대기 중 여자 전류 차단(발열 방지)
        if (dispense_servo_pending) { dispense_servo_pending = 0; Servo_Open_Close_Async(); }
    }
    // s2, servo도 동일한 패턴으로 처리
}
```

**동작 체이닝** — 알람이 울리면 `Dispense_Rotate_Async()` 하나만 호출하고, 나머지는 태스크가 이어받습니다.

```
RTC 알람 → Dispense_Rotate_Async()
        → (s1 회전 완료) → Servo_Open_Close_Async()
        → (서보 개방 500ms) → conveyor_start_pending = 1
        → 메인 루프가 Conveyor_Start() 호출 → FSM 진입
```

기존 버전이 `Rotate_Next_Slot(); Servo_Open_Close(); for(...0x100000);`로 순차 대기하던 구간이, 전부 비동기 상태 전이로 바뀌었습니다. 서보는 `1400 → (2000ms 유지) → 2000 → CCR1=0` 4단계로 진행되며 모터 정지 시 PWM을 끊어 지터를 없앴습니다.

### 3. 버튼 입력을 논블로킹 디바운스로 교체 ([`key.c`](./key.c))

기존 `STATE_WAIT` 처리는 `Key_Wait_Key_Released()`로 **버튼에서 손을 뗄 때까지 시스템 전체가 정지**했습니다. 개선판은 20ms 안정화 조건을 만족한 상승 엣지만 이벤트로 저장하고, FSM은 그 이벤트를 소비만 합니다.

```c
if (key_candidate != key_stable &&
    (Get_Tick() - key_change_tick) >= KEY_DEBOUNCE_MS) {
    key_stable = key_candidate;
    if (key_stable) key_event = 1;   // Key_Get_Pressed()가 1회 소비
}
```

### 4. 명령 파싱과 실행 분리 ([`command_parser.c`](./command_parser.c))

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

잘못된 명령에는 `Uart1_Printf("ERR\n")`으로 앱에 즉시 피드백합니다.

**지원 명령**

| 명령 | 예시 | 동작 |
|---|---|---|
| `T HH:MM:SS` | `T08:30:00` | RTC 현재 시각 설정, 앱에 `T:08:30:00` 에코 |
| `A HH:MM:SS` | `A09:00:00` | 복약 알람 등록, 앱에 `A:09:00:00` 에코 |
| `L` + 7자리 | `L1010100` | 7일 적재 시퀀스 예약, 앱에 `L:OK` 응답 |
| `f` / `F` | `f` | 수동 배출 — 슬롯 회전 + 서보 + 컨베이어 시작 |
| `servo` | `servo` | 배출 서보 단독 트리거 |
| `step` / `s` | `step` | 슬롯 스텝모터 300스텝 이동 (테스트) |
| `store` | `store` | 7칸 일괄 적재 (테스트) |
| 그 외 | — | `CMD_INVALID` → `ERR` 응답 |

UART1은 전용 ISR에서 `\r`/`\n`을 만날 때까지 64바이트 버퍼에 누적하며, 직전 명령이 아직 처리되지 않았으면 새 문자를 버려 버퍼 훼손을 막습니다.

### 5. Auto-Load 시퀀스 동작 수정

기존 구현은 `Supply_Pill()` 다음 `Stepper2_One_Day()`를 호출했는데, **둘 다 호퍼 스텝모터(s2)** 를 돌리고 있어 메인 약통이 회전하지 않았습니다. 개선판은 약통(s1)과 호퍼(s2)를 명확히 분리했습니다.

```c
for (int i = 0; i < 7; i++) {
    Rotate_Next_Slot_Async();   // s1: 메인 약통 1칸
    Wait_Motor_Done();
    if (week_data[i] == '1') {
        Status_LED_Red();
        Supply_Pill_Async();    // s2: 호퍼 공급
        Wait_Motor_Done();
        Status_LED_All_Off();
    }
}
```

> ⚠️ 이 시퀀스는 슬롯 순서 보장을 위해 `Wait_Motor_Done()`으로 대기합니다. 다만 대기 중에도 `Motor_Update_Task()`와 `IWDG_Refresh()`를 계속 호출하므로 워치독 리셋은 발생하지 않습니다. 완전 논블로킹은 아니며, 적재 단계를 FSM으로 옮기는 것이 다음 개선 과제입니다.

### 6. 고장 시 안전 상태로 전이 ([`conveyor_fsm.c`](./conveyor_fsm.c))

`Ultrasonic_Get_Distance()`는 에코 무응답 시 `-1`, 에코 타임아웃 시 `-2`를 반환합니다. FSM은 음수 거리를 감지하면 즉시 안전 정지 후 `STATE_ERROR`로 전이합니다.

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

### 7. 시스템 보호 설정

- **워치독**: `IWDG_Init()` — LSI 32kHz, PR=4(÷64), RLR=1000 → **약 2초** 타임아웃. 메인 루프가 멈추면 MCU가 자동 리셋
- **인터럽트 우선순위**: `NVIC_Config()`에서 그룹 3으로 RTC Alarm(0) → USART1(1) → USART2(2) 순 명시. RTC 알람이 통신에 밀리지 않도록 보장
- **컨베이어 듀티**: `Motor_Set_Percent()`가 20~100% 범위로 클램핑해 기동 토크 부족 구간을 차단

```c
void NVIC_Config(void)
{
    NVIC_SetPriorityGrouping(3);
    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(3, 0, 0));
    NVIC_SetPriority(USART1_IRQn,    NVIC_EncodePriority(3, 1, 0));
    NVIC_SetPriority(USART2_IRQn,    NVIC_EncodePriority(3, 2, 0));
}
```

---

## 개선의 의미

- **응답성**: 모터·서보 동작 중에도 통신·센서·화면 갱신·워치독이 계속 진행됨
- **유지보수성**: `main.c`가 383줄 → 177줄로 줄고, 기능별 파일 경계가 명확해 수정 영향 범위가 축소됨
- **안전성**: 입력 오류(`CMD_INVALID`), 센서 이상(`STATE_ERROR`), 루프 정지(IWDG)에 각각 대응하는 방어 로직 확보
- **정확성**: Auto-Load 모터 오배정 수정, `T`/`A` 인자 파싱 지원

---

## 빌드

`Final_Project`와 동일한 레지스터 직접 제어 방식이며 HAL 의존성은 없습니다.

```bash
mv Makefile.txt Makefile   # 최초 1회
make                       # arm-none-eabi-gcc, Cortex-M4 hard-float
make flash                 # STM32_Programmer_CLI (SWD)
```

Makefile은 `wildcard *.c`로 소스를 수집하므로 새 모듈은 추가만 하면 자동으로 빌드에 포함됩니다.

---

*원본 통합형 구현은 [`../Final_Project`](../Final_Project)와 [루트 README](../README.md)를 참고하세요.*

#ifndef PIN_MAP_H
#define PIN_MAP_H

/*
 * 하드웨어 핀 매핑 중앙 관리
 *
 * 각 드라이버는 이 헤더를 include 해서 핀 번호를 참조한다.
 * 배선이 바뀌면 이 파일만 수정하면 된다.
 *
 * 레지스터 필드 폭이 핀마다 다르므로 아래 헬퍼를 사용한다.
 *   MODER / OTYPER(1bit) / OSPEEDR / PUPDR : 핀당 2비트  -> PIN_POS2()
 *   AFR[0]=핀0~7, AFR[1]=핀8~15            : 핀당 4비트  -> PIN_AFR_IDX/POS()
 */
#define PIN_POS2(n)      ((n) * 2)
#define PIN_AFR_IDX(n)   ((n) >> 3)
#define PIN_AFR_POS(n)   (((n) & 0x7) * 4)

/* RCC->AHB1ENR 비트 (GPIO 포트 클럭) */
#define RCC_GPIOA_BIT    0
#define RCC_GPIOB_BIT    1
#define RCC_GPIOC_BIT    2

/* ── 외부 버튼 : WAIT 상태에서 복약 확인 입력 (Pull-up, Active Low) ── */
#define PIN_BUTTON_PORT        GPIOB
#define PIN_BUTTON_NUM         5
#define PIN_BUTTON_RCC_BIT     RCC_GPIOB_BIT

/* ── 패시브 부저 : 복약 알림음 (TIM4_CH1) ── */
#define PIN_BUZZER_PORT        GPIOB
#define PIN_BUZZER_NUM         6
#define PIN_BUZZER_AF          2
#define PIN_BUZZER_RCC_BIT     RCC_GPIOB_BIT

/* ── 초음파 HC-SR04 : 컨베이어 위 약 위치 측정 ── */
#define PIN_ULTRA_PORT         GPIOC
#define PIN_ULTRA_TRIG         4
#define PIN_ULTRA_ECHO         5
#define PIN_ULTRA_RCC_BIT      RCC_GPIOC_BIT

/* ── 스텝모터1 : 메인 약통 요일 슬롯 회전 (PC0~PC3) ── */
#define PIN_STEPPER1_PORT      GPIOC
#define PIN_STEPPER1_SHIFT     0
#define PIN_STEPPER1_MASK      (0xFu << PIN_STEPPER1_SHIFT)
#define PIN_STEPPER1_RCC_BIT   RCC_GPIOC_BIT

/* ── 스텝모터2 : 상부 호퍼 약 공급 (PC6~PC9) ── */
#define PIN_STEPPER2_PORT      GPIOC
#define PIN_STEPPER2_SHIFT     6
#define PIN_STEPPER2_MASK      (0xFu << PIN_STEPPER2_SHIFT)
#define PIN_STEPPER2_RCC_BIT   RCC_GPIOC_BIT

/* ── DC 모터 : 컨베이어 정/역회전 (TIM5_CH1 / TIM5_CH2) ── */
#define PIN_DCMOTOR_PORT       GPIOA
#define PIN_DCMOTOR_CW_NUM     0
#define PIN_DCMOTOR_CCW_NUM    1
#define PIN_DCMOTOR_AF         2
#define PIN_DCMOTOR_RCC_BIT    RCC_GPIOA_BIT

/* ── 서보모터 : 배출구 개폐 (TIM3_CH1) ── */
#define PIN_SERVO_PORT         GPIOA
#define PIN_SERVO_NUM          6
#define PIN_SERVO_AF           2
#define PIN_SERVO_RCC_BIT      RCC_GPIOA_BIT

/* ── 상태 LED (Active Low : ODR=0 이면 점등) ── */
#define PIN_STATUS_RED_PORT    GPIOB
#define PIN_STATUS_RED_NUM     12
#define PIN_STATUS_RED_RCC_BIT RCC_GPIOB_BIT
#define PIN_STATUS_GRN_PORT    GPIOA
#define PIN_STATUS_GRN_NUM     7
#define PIN_STATUS_GRN_RCC_BIT RCC_GPIOA_BIT

/* ── I2C1 : LCD 2대 공용 버스 (PB8 SCL / PB9 SDA) ── */
#define PIN_I2C_PORT           GPIOB
#define PIN_I2C_SCL_NUM        8
#define PIN_I2C_SDA_NUM        9
#define PIN_I2C_AF             4
#define PIN_I2C_RCC_BIT        RCC_GPIOB_BIT

/* ── LCD I2C 슬레이브 주소 (PCF8574) ── */
#define LCD1_ADDR              0x4E   /* 0x27 << 1 : 현재 시각 / 알람 표시 */
#define LCD2_ADDR              0x4C   /* 0x26 << 1 : 컨베이어 상태 표시   */

#endif

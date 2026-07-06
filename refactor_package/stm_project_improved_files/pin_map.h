#ifndef PIN_MAP_H
#define PIN_MAP_H

/*
 * pin_map.h
 * ------------------------------------------------------------
 * 프로젝트에서 사용하는 핀 정보를 한 곳에 모아둔 파일입니다.
 *
 * 현재 핀맵은 기존 Final_Project 코드 기준입니다.
 */

#define PIN_BUTTON_PORT     GPIOB
#define PIN_BUTTON_NUM      5

#define PIN_BUZZER_PORT     GPIOB
#define PIN_BUZZER_NUM      6

#define PIN_ULTRA_PORT      GPIOC
#define PIN_ULTRA_TRIG      4
#define PIN_ULTRA_ECHO      5

#define PIN_STEPPER1_PORT   GPIOC
#define PIN_STEPPER1_SHIFT  0       // PC0~PC3

#define PIN_STEPPER2_PORT   GPIOC
#define PIN_STEPPER2_SHIFT  6       // PC6~PC9

#define PIN_STATUS_RED_PORT GPIOB
#define PIN_STATUS_RED_NUM  12

#define PIN_STATUS_GREEN_PORT GPIOA
#define PIN_STATUS_GREEN_NUM  7

#define LCD1_ADDR 0x4E
#define LCD2_ADDR 0x4C

#endif

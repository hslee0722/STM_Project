#ifndef APP_STATE_H
#define APP_STATE_H

/*
 * app_state.h
 * ------------------------------------------------------------
 * 여러 파일에서 공통으로 사용하는 상태값과 플래그를 정리한 파일입니다.
 */

typedef enum {
    STATE_IDLE = 0,
    STATE_FORWARD,
    STATE_WAIT,
    STATE_BACKWARD,
    STATE_FINISHED,
    STATE_ERROR
} ConveyorState;

extern volatile int pill_alarm_flag;
extern volatile int auto_load_flag;
extern char auto_load_data[8];

#endif

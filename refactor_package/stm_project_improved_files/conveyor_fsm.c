#include "device_driver.h"
#include "conveyor_fsm.h"
#include <stdio.h>

/*
 * conveyor_fsm.c
 * ------------------------------------------------------------
 * 기존 main.c에 있던 컨베이어 상태 머신을 분리한 파일입니다.
 *
 * 상태 흐름:
 * IDLE
 *   → Conveyor_Start()
 * FORWARD
 *   → 초음파 거리 <= 1cm 감지 시 정지 + 부저 ON
 * WAIT
 *   → PB5 외부 스위치 입력 시 부저 OFF + 역방향
 * BACKWARD
 *   → 거리 >= 15cm 감지 시 정지 + 완료 처리
 * FINISHED
 */

static ConveyorState current_state = STATE_IDLE;

void Conveyor_Init_State(void)
{
    current_state = STATE_IDLE;
}

ConveyorState Conveyor_Get_State(void)
{
    return current_state;
}

void Conveyor_Start(void)
{
    if (current_state == STATE_IDLE || current_state == STATE_FINISHED) {
        printf("[CONVEYOR] Start Forward\r\n");
        Status_LED_Green();
        Motor_Set_Percent(50);
        Move_CW();
        current_state = STATE_FORWARD;
    }
}

/*
 * Conveyor_Update()
 * ------------------------------------------------------------
 * main loop에서 주기적으로 호출합니다.
 *
 * dist:
 *   Ultrasonic_Get_Distance()에서 읽은 거리값
 *
 * 안전 개선:
 *   dist가 -1 또는 -2이면 초음파 센서 오류로 판단하고 모터 정지
 */
void Conveyor_Update(int dist)
{
    if (dist < 0) {
        Stop();
        Buzzer_Off();
        Status_LED_All_Off();
        current_state = STATE_ERROR;
        printf("\r\n[ERROR] Ultrasonic sensor error: %d\r\n", dist);
        return;
    }

    switch (current_state)
    {
        case STATE_IDLE:
        case STATE_FINISHED:
            break;

        case STATE_FORWARD:
            if (dist > 0 && dist <= 1) {
                Stop();
                Buzzer_On();
                Status_LED_All_Off();

                printf("\r\n[CONVEYOR] Arrived. dist=%d cm\r\n", dist);
                printf("[CONVEYOR] Press external switch(PB5) to return.\r\n");

                current_state = STATE_WAIT;
            }
            break;

        case STATE_WAIT:
            if (Key_Get_Pressed()) {
                Buzzer_Off();

                printf("[CONVEYOR] Button pressed. Return backward.\r\n");
                Status_LED_Red();

                Motor_Set_Percent(50);
                Move_CCW();

                Key_Wait_Key_Released();
                TIM2_Delay(50);

                current_state = STATE_BACKWARD;
            }
            break;

        case STATE_BACKWARD:
            if (dist >= 15) {
                Stop();
                Buzzer_Off();
                Status_LED_All_Off();

                printf("\r\n[CONVEYOR] Returned. Sequence finished.\r\n");

                /*
                 * 기존 main.c에서는 복귀 완료 후
                 * Rotate_Next_Slot(); Supply_Pill();을 실행했습니다.
                 * 기구 의도에 따라 유지/삭제를 결정하세요.
                 */
                Rotate_Next_Slot();
                TIM2_Delay(500);
                Supply_Pill();

                current_state = STATE_FINISHED;
            }
            break;

        case STATE_ERROR:
            /*
             * 에러 상태에서는 자동 재시작하지 않습니다.
             * 재시작하려면 리셋 명령 또는 별도 복구 명령을 추가하는 것이 안전합니다.
             */
            break;

        default:
            current_state = STATE_ERROR;
            break;
    }
}

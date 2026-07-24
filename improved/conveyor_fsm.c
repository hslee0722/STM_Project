#include "device_driver.h"
#include "conveyor_fsm.h"

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
        Status_LED_Green();
        Motor_Set_Percent(50);
        Move_CW();
        current_state = STATE_FORWARD;
    }
}

void Conveyor_Update(int dist)
{
    if (dist < 0) {
        Stop();
        Buzzer_Off();
        Status_LED_All_Off();
        current_state = STATE_ERROR;
        return;
    }

    switch (current_state)
    {
        case STATE_IDLE:
        case STATE_FINISHED:
        case STATE_ERROR:
            break;

        case STATE_FORWARD:
            if (dist > 0 && dist <= 1) {
                Stop();
                Buzzer_On();
                Status_LED_All_Off();
                current_state = STATE_WAIT;
            }
            break;

        case STATE_WAIT:
            if (Key_Get_Pressed()) {
                Buzzer_Off();
                Status_LED_Red();
                Motor_Set_Percent(50);
                Move_CCW();
                current_state = STATE_BACKWARD;
            }
            break;

        case STATE_BACKWARD:
            if (dist >= 15) {
                Stop();
                Buzzer_Off();
                Status_LED_All_Off();
                Rotate_Then_Supply_Async();
                current_state = STATE_FINISHED;
            }
            break;

        default:
            current_state = STATE_ERROR;
            break;
    }
}

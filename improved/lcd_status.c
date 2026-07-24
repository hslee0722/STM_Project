#include "device_driver.h"
#include "app_state.h"
#include <stdio.h>
#include "pin_map.h"

/* LCD2 : 컨베이어 진행 상황 표시 (16x2) */
void LCD2_Show_State(int state, int dist)
{
    char line2[17];

    LCD_Set_Cursor_To(LCD2_ADDR, 0, 0);

    switch (state)
    {
        case STATE_IDLE:
            LCD_String_To(LCD2_ADDR, "State: IDLE    ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Waiting Alarm  ");
            return;

        case STATE_FORWARD:
            LCD_String_To(LCD2_ADDR, "State: FORWARD ");
            sprintf(line2, "Moving  %3dcm  ", dist);
            break;

        case STATE_WAIT:
            LCD_String_To(LCD2_ADDR, "State: WAIT BTN");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Press Button   ");
            return;

        case STATE_BACKWARD:
            LCD_String_To(LCD2_ADDR, "State: BACKWARD");
            sprintf(line2, "Return  %3dcm  ", dist);
            break;

        case STATE_FINISHED:
            LCD_String_To(LCD2_ADDR, "State: DONE    ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Sequence End   ");
            return;

        case STATE_ERROR:
            /* 초음파 무응답(-1) / 에코 타임아웃(-2) */
            LCD_String_To(LCD2_ADDR, "State: ERROR   ");
            sprintf(line2, "Sensor Fault %-2d", dist);
            break;

        default:
            LCD_String_To(LCD2_ADDR, "State: UNKNOWN ");
            LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
            LCD_String_To(LCD2_ADDR, "Check System   ");
            return;
    }

    LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
    LCD_String_To(LCD2_ADDR, line2);
}

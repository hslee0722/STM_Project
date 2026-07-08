#include "device_driver.h"
#include "command_parser.h"
#include "app_state.h"
#include <stdio.h>
#include <string.h>

static int is_week_pattern(const char *s)
{
    for (int i = 0; i < 7; i++) {
        if (s[i] != '0' && s[i] != '1') return 0;
    }
    return 1;
}

ParsedCommand Command_Parse(const char *rx)
{
    ParsedCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_NONE;

    if (rx == 0 || rx[0] == '\0') {
        return cmd;
    }

    if (rx[0] == 'T' || rx[0] == 't') {
        cmd.type = (sscanf(rx + 1, "%d:%d:%d", &cmd.h, &cmd.m, &cmd.s) == 3) ? CMD_SET_TIME : CMD_INVALID;
        return cmd;
    }
    if (rx[0] == 'A' || rx[0] == 'a') {
        cmd.type = (sscanf(rx + 1, "%d:%d:%d", &cmd.h, &cmd.m, &cmd.s) == 3) ? CMD_SET_ALARM : CMD_INVALID;
        return cmd;
    }
    if (rx[0] == 'L' || rx[0] == 'l') {
        if (strlen(rx) >= 8 && is_week_pattern(rx + 1)) {
            strncpy(cmd.week_data, rx + 1, 7);
            cmd.week_data[7] = '\0';
            cmd.type = CMD_AUTO_LOAD;
        } else {
            cmd.type = CMD_INVALID;
        }
        return cmd;
    }
    if (strcmp(rx, "f") == 0 || strcmp(rx, "F") == 0) { cmd.type = CMD_MANUAL_DROP;  return cmd; }
    if (strcmp(rx, "servo") == 0)                     { cmd.type = CMD_SERVO_TEST;   return cmd; }
    if (strcmp(rx, "step") == 0 || strcmp(rx, "s") == 0) { cmd.type = CMD_STEPPER_TEST; return cmd; }
    if (strcmp(rx, "store") == 0)                     { cmd.type = CMD_STORE_TEST;   return cmd; }

    cmd.type = CMD_INVALID;
    return cmd;
}

void Command_Execute(const ParsedCommand *cmd)
{
    if (cmd == 0) return;

    switch (cmd->type) {
        case CMD_SET_TIME:
            Set_Current_Time(cmd->h, cmd->m, cmd->s);
            Uart1_Printf("T:%02d:%02d:%02d\n", cmd->h, cmd->m, cmd->s);
            break;

        case CMD_SET_ALARM: {
            char alarm_str[16];
            sprintf(alarm_str, "%02d:%02d:%02d", cmd->h, cmd->m, cmd->s);
            Set_Alarm_From_String(alarm_str);
            Uart1_Printf("A:%02d:%02d:%02d\n", cmd->h, cmd->m, cmd->s);
            break;
        }

        case CMD_AUTO_LOAD:
            strncpy(auto_load_data, cmd->week_data, 7);
            auto_load_data[7] = '\0';
            auto_load_flag = 1;
            Uart1_Printf("L:OK\n");
            break;

        case CMD_SERVO_TEST:
            Servo_Open_Close_Async();
            break;

        case CMD_STEPPER_TEST: {
            static int current_step = 0;
            for (int i = 0; i < 300; i++) {
                Stepper_Step(current_step++);
                TIM2_Delay(5);
            }
            GPIOC->ODR &= ~(0xF << 0);
            break;
        }

        case CMD_STORE_TEST:
            for (int i = 0; i < 7; i++) {
                Stepper2_One_Day_Async();
                Supply_Pill_Async();
                TIM2_Delay(5);
            }
            Rotate_Next_Slot_Async();
            break;

        case CMD_INVALID:
            Uart1_Printf("ERR\n");
            break;

        default:
            break;
    }
}

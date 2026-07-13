#include "device_driver.h"
#include "command_parser.h"
#include "app_state.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 * is_week_pattern()
 * ------------------------------------------------------------
 * L1010100 같은 자동 적재 명령에서 7자리 데이터가
 * 모두 '0' 또는 '1'인지 검사합니다.
 */
static int is_week_pattern(const char *s)
{
    for (int i = 0; i < 7; i++) {
        if (s[i] != '0' && s[i] != '1') return 0;
    }
    return 1;
}

/*
 * Command_Parse()
 * ------------------------------------------------------------
 * 문자열 명령을 ParsedCommand 구조체로 변환합니다.
 *
 * 예:
 * "T08:30:00" -> CMD_SET_TIME, h=8, m=30, s=0
 * "A09:00:00" -> CMD_SET_ALARM, h=9, m=0, s=0
 * "L1010100"  -> CMD_AUTO_LOAD, week_data="1010100"
 * "f"         -> CMD_MANUAL_DROP
 * "servo"     -> CMD_SERVO_TEST
 * "step"      -> CMD_STEPPER_TEST
 */
ParsedCommand Command_Parse(const char *rx)
{
    ParsedCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.type = CMD_NONE;

    if (rx == 0 || rx[0] == '\0') {
        cmd.type = CMD_NONE;
        return cmd;
    }

    if (rx[0] == 'T' || rx[0] == 't') {
        if (sscanf(rx + 1, "%d:%d:%d", &cmd.h, &cmd.m, &cmd.s) == 3) {
            cmd.type = CMD_SET_TIME;
        } else {
            cmd.type = CMD_INVALID;
        }
        return cmd;
    }

    if (rx[0] == 'A' || rx[0] == 'a') {
        if (sscanf(rx + 1, "%d:%d:%d", &cmd.h, &cmd.m, &cmd.s) == 3) {
            cmd.type = CMD_SET_ALARM;
        } else {
            cmd.type = CMD_INVALID;
        }
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

    if (strcmp(rx, "f") == 0 || strcmp(rx, "F") == 0) {
        cmd.type = CMD_MANUAL_DROP;
        return cmd;
    }

    if (strcmp(rx, "servo") == 0) {
        cmd.type = CMD_SERVO_TEST;
        return cmd;
    }

    if (strcmp(rx, "step") == 0 || strcmp(rx, "s") == 0) {
        cmd.type = CMD_STEPPER_TEST;
        return cmd;
    }

    if (strcmp(rx, "store") == 0) {
        cmd.type = CMD_STORE_TEST;
        return cmd;
    }

    cmd.type = CMD_INVALID;
    return cmd;
}

/*
 * Command_Execute()
 * ------------------------------------------------------------
 * 파싱된 명령을 실제 동작으로 연결합니다.
 *
 * 주의:
 * 모터를 오래 돌리는 작업은 ISR 안에서 직접 호출하지 않는 것이 좋습니다.
 * UART 인터럽트에서는 문자열만 만들고,
 * main loop에서 Command_Parse/Execute를 호출하는 구조를 권장합니다.
 */
void Command_Execute(const ParsedCommand *cmd)
{
    if (cmd == 0) return;

    switch (cmd->type) {
        case CMD_SET_TIME:
            Set_Current_Time(cmd->h, cmd->m, cmd->s);
            printf("\r\n[SYNC] Current Time -> %02d:%02d:%02d\r\n", cmd->h, cmd->m, cmd->s);
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
            printf("\r\n[BLE] Auto-Load Command: %s\r\n", auto_load_data);
            Uart1_Printf("L:OK\n");
            break;

        case CMD_SERVO_TEST:
            Servo_Open_Close();
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
                Stepper2_One_Day();
                Supply_Pill();
                TIM2_Delay(5);
            }
            Rotate_Next_Slot();
            break;

        case CMD_INVALID:
            printf("\r\n[ERROR] Invalid command\r\n");
            Uart1_Printf("ERR\n");
            break;

        default:
            break;
    }
}

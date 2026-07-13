#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

/*
 * command_parser.h
 * ------------------------------------------------------------
 * UART/BLE/PC에서 들어온 문자열 명령을 해석하는 파서입니다.
 */

typedef enum {
    CMD_NONE = 0,
    CMD_SET_TIME,
    CMD_SET_ALARM,
    CMD_AUTO_LOAD,
    CMD_MANUAL_DROP,
    CMD_SERVO_TEST,
    CMD_STEPPER_TEST,
    CMD_STORE_TEST,
    CMD_INVALID
} CommandType;

typedef struct {
    CommandType type;
    int h;
    int m;
    int s;
    char week_data[8];   // "1010100" + '\0'
} ParsedCommand;

ParsedCommand Command_Parse(const char *rx);
void Command_Execute(const ParsedCommand *cmd);

#endif

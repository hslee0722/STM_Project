#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

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
    char week_data[8];
} ParsedCommand;

ParsedCommand Command_Parse(const char *rx);
void Command_Execute(const ParsedCommand *cmd);

#endif

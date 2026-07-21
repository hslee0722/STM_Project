#ifndef APP_STATE_H
#define APP_STATE_H

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

extern volatile char uart1_buffer[64];
extern volatile int  uart1_rx_exist;
extern volatile char uart2_buffer[64];
extern volatile int  uart2_rx_exist;

extern volatile int conveyor_start_pending;
extern void Dispense_Rotate_Async(void);
extern int  Motor_All_Idle(void);

#endif

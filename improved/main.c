#include "device_driver.h"
#include "app_state.h"
#include "conveyor_fsm.h"
#include "command_parser.h"
#include "nvic_config.h"
#include "iwdg.h"
#include <stdio.h>
#include <string.h>

#define LCD1_ADDR 0x4E
#define LCD2_ADDR 0x4C

volatile int system_mode = 0;
volatile int pill_alarm_flag = 0;
volatile int auto_load_flag = 0;
char auto_load_data[8] = "0000000";

static void Sys_Init(void)
{
    SCB->CPACR |= (0x3 << 10 * 2) | (0x3 << 11 * 2);
    Clock_Init();
    Tick_Init();
    NVIC_Config();
    Uart2_Init(115200);
    Uart1_Init(9600);
    setvbuf(stdout, NULL, _IONBF, 0);

    LED_Init();
    Status_LED_Init();
    Motor_Init();
    LCD_Init();
    LCD_Init_To(LCD2_ADDR);
    Ultrasonic_Init();
    Key_Poll_Init();
    Buzzer_Init();
    Uart2_RX_Interrupt_Enable();

    Conveyor_Init_State();
}

static void Auto_Load_Sequence(const char *week_data)
{
    LCD_Set_Cursor_To(LCD2_ADDR, 0, 0);
    LCD_String_To(LCD2_ADDR, "State: LOADING ");

    for (int i = 0; i < 7; i++) {
        char lcd_buf[20];
        sprintf(lcd_buf, "Slot %d: %s      ", i + 1, (week_data[i] == '1') ? "LOADING" : "SKIP");
        LCD_Set_Cursor_To(LCD2_ADDR, 1, 0);
        LCD_String_To(LCD2_ADDR, lcd_buf);

        if (week_data[i] == '1') {
            Status_LED_Red();
            Supply_Pill_Async();
            Delay_ms(1500);
            IWDG_Refresh();
        }
        Status_LED_All_Off();
        Stepper2_One_Day_Async();
        Delay_ms(1500);
        IWDG_Refresh();
    }
}

static void Handle_Pill_Alarm(void)
{
    Status_LED_Red();
    Rotate_Next_Slot_Async();   
    pill_alarm_flag = 0;
    Conveyor_Start();
}

static void Handle_Command(const char *buf)
{
    ParsedCommand cmd = Command_Parse(buf);

    if (cmd.type == CMD_MANUAL_DROP) {
        Status_LED_Green();
        Rotate_Next_Slot_Async();
        Servo_Open_Close_Async();
        Conveyor_Start();
    } else {
        Command_Execute(&cmd);
    }
}

static void Handle_Uart1_Command(void)
{
    if (!uart1_rx_exist) return;
    Handle_Command((const char *)uart1_buffer);
    uart1_buffer[0] = '\0';
    uart1_rx_exist = 0;
}

static void Handle_Uart2_Command(void)
{
    if (!uart2_rx_exist) return;
    Handle_Command((const char *)uart2_buffer);
    uart2_buffer[0] = '\0';
    uart2_rx_exist = 0;
}

static void Update_Clock_Display(void)
{
    static int last_sec = -1;
    unsigned int tr = RTC->TR;
    int s = BCD_TO_DEC(tr & 0x7F);
    if (s == last_sec) return;

    int m = BCD_TO_DEC((tr >> 8) & 0x7F);
    int h = BCD_TO_DEC((tr >> 16) & 0x3F);
    unsigned int alr = RTC->ALRMAR;
    int as = BCD_TO_DEC(alr & 0x7F);
    int am = BCD_TO_DEC((alr >> 8) & 0x7F);
    int ah = BCD_TO_DEC((alr >> 16) & 0x3F);

    char b[20];
    sprintf(b, "Clock : %02d:%02d:%02d", h, m, s);
    LCD_Set_Cursor(0, 0); LCD_String(b);
    sprintf(b, "Alarm : %02d:%02d:%02d", ah, am, as);
    LCD_Set_Cursor(1, 0); LCD_String(b);

    last_sec = s;
}

void Main(void)
{
    Sys_Init();
    RTC_Init_And_Alarm_Set(0, 0, 0);
    IWDG_Init();

    uint32_t last_sensor_time = 0;
    int dist = 0;

    Status_LED_All_Off();

    while (1) {
        IWDG_Refresh();
        Motor_Update_Task();

        Handle_Uart1_Command();
        Handle_Uart2_Command();

        if (auto_load_flag == 1) {
            Auto_Load_Sequence(auto_load_data);
            auto_load_flag = 0;
            Conveyor_Init_State();
        }

        Update_Clock_Display();

        if (pill_alarm_flag == 1) {
            Handle_Pill_Alarm();
        }

        if (Get_Tick() - last_sensor_time >= 100) {
            last_sensor_time = Get_Tick();
            dist = Ultrasonic_Get_Distance();
            Conveyor_Update(dist);
            LCD2_Show_State(Conveyor_Get_State(), dist);
        }
    }
}

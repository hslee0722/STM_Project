#include "device_driver.h"
#include "app_state.h"
#include "conveyor_fsm.h"
#include "command_parser.h"
#include "nvic_config.h"
#include <stdio.h>
#include <string.h>

/*
 * main_refactor_example.c
 * ------------------------------------------------------------
 * 기존 main.c를 전부 대체하기보다는,
 * main.c를 어떻게 줄이면 좋은지 보여주는 개선 예시입니다.
 *
 * 실제 적용 시에는 기존 main.c와 비교하면서 옮기세요.
 */

volatile int system_mode = 0;
volatile int pill_alarm_flag = 0;
volatile int auto_load_flag = 0;
char auto_load_data[8] = "0000000";

volatile unsigned char Uart_Data = 0;
volatile char uart2_buffer[64];
volatile int uart2_rx_index = 0;
volatile int uart2_rx_exist = 0;

static void Sys_Init(void)
{
    SCB->CPACR |= (0x3 << 10*2) | (0x3 << 11*2);

    Clock_Init();

    /*
     * NVIC 우선순위는 주변장치 초기화 전에 한 번 명확히 설정합니다.
     */
    NVIC_Config();

    Uart2_Init(115200);
    Uart1_Init(9600);
    setvbuf(stdout, NULL, _IONBF, 0);

    LED_Init();
    Status_LED_Init();
    Motor_Init();

    LCD_Init();
    LCD_Init_To(0x4C);

    Ultrasonic_Init();
    Key_Poll_Init();
    Buzzer_Init();

    Uart2_RX_Interrupt_Enable();

    Conveyor_Init_State();
}

static void Auto_Load_Sequence(const char* week_data)
{
    printf("\r\n[LOADING] Start Auto-Loading Sequence for 7 days...\r\n");

    LCD_Set_Cursor_To(0x4C, 0, 0);
    LCD_String_To(0x4C, "State: LOADING ");

    for (int i = 0; i < 7; i++) {
        char lcd_buf[20];

        sprintf(lcd_buf, "Slot %d: %s      ",
                i + 1,
                (week_data[i] == '1') ? "LOADING" : "SKIP");

        LCD_Set_Cursor_To(0x4C, 1, 0);
        LCD_String_To(0x4C, lcd_buf);

        printf("[LOADING] Slot %d -> %s\r\n",
               i + 1,
               (week_data[i] == '1') ? "LOADING" : "SKIP");

        if (week_data[i] == '1') {
            Status_LED_Red();
            Supply_Pill();
            TIM2_Delay(500);
        }

        Status_LED_All_Off();

        /*
         * 이름은 Stepper2_One_Day이지만 실제 motor.c 내부에서는
         * Stepper_Step()을 호출하므로 첫 번째 스텝모터 쪽이 돌 수 있습니다.
         * 추후 함수명을 Rotate_Day_Slot()으로 바꾸는 것을 추천합니다.
         */
        Stepper2_One_Day();
        TIM2_Delay(500);
    }

    printf("[LOADING] Sequence Complete!\r\n");
    LCD_Set_Cursor_To(0x4C, 0, 0);
    LCD_String_To(0x4C, "State: LOAD DONE");
}

/*
 * RTC 알람 발생 시 약 배출 동작
 */
static void Handle_Pill_Alarm(void)
{
    printf("\r\n[ACTION] Pill alarm triggered.\r\n");

    Status_LED_Red();

    Rotate_Next_Slot();
    Servo_Open_Close();

    pill_alarm_flag = 0;

    Conveyor_Start();
}

/*
 * PC 디버그 UART2 명령 처리
 */
static void Handle_Debug_Command(void)
{
    if (!uart2_rx_exist) return;

    ParsedCommand cmd = Command_Parse((const char *)uart2_buffer);

    if (cmd.type == CMD_MANUAL_DROP) {
        printf("\r\n[ACTION] Manual pill drop\r\n");
        Status_LED_Green();
        Rotate_Next_Slot();
        Servo_Open_Close();
        Conveyor_Start();
    } else {
        Command_Execute(&cmd);
    }

    uart2_buffer[0] = '\0';
    uart2_rx_exist = 0;
}

/*
 * LCD1 현재 시간/알람 표시
 */
static void Update_Clock_Display(void)
{
    static int last_sec = -1;

    unsigned int tr = RTC->TR;

    int s = BCD_TO_DEC(tr & 0x7F);
    int m = BCD_TO_DEC((tr >> 8) & 0x7F);
    int h = BCD_TO_DEC((tr >> 16) & 0x3F);

    if (s == last_sec) return;

    unsigned int alr = RTC->ALRMAR;
    int as = BCD_TO_DEC(alr & 0x7F);
    int am = BCD_TO_DEC((alr >> 8) & 0x7F);
    int ah = BCD_TO_DEC((alr >> 16) & 0x3F);

    printf("\r[PC] Time %02d:%02d:%02d | Alarm %02d:%02d:%02d  ",
           h, m, s, ah, am, as);

    char lcd_buffer[20];

    sprintf(lcd_buffer, "Clock : %02d:%02d:%02d", h, m, s);
    LCD_Set_Cursor(0, 0);
    LCD_String(lcd_buffer);

    sprintf(lcd_buffer, "Alarm : %02d:%02d:%02d", ah, am, as);
    LCD_Set_Cursor(1, 0);
    LCD_String(lcd_buffer);

    last_sec = s;
}

void Main(void)
{
    Sys_Init();

    /*
     * 초기 RTC는 00:00:00, 알람도 00:00:00으로 시작.
     * 실제 사용 시 BLE 앱에서 T/A 명령으로 동기화하는 것을 권장합니다.
     */
    RTC_Init_And_Alarm_Set(0, 0, 0);

    printf("=== Integrated Pill Dispenser System Ready ===\r\n");

    Status_LED_All_Off();

    while (1)
    {
        /*
         * BLE UART1 명령은 기존 uart.c의 USART1_IRQHandler에서 처리됩니다.
         * 개선하려면 UART1도 ISR에서 문자열만 만들고,
         * 여기 main loop에서 Command_Parse/Execute를 호출하도록 통합하세요.
         */

        Handle_Debug_Command();

        if (auto_load_flag == 1) {
            Auto_Load_Sequence(auto_load_data);
            auto_load_flag = 0;
            Conveyor_Init_State();
        }

        Update_Clock_Display();

        if (pill_alarm_flag == 1) {
            Handle_Pill_Alarm();
        }

        int dist = Ultrasonic_Get_Distance();

        Conveyor_Update(dist);
        LCD2_Show_State(Conveyor_Get_State(), dist);

        Delay_ms(100);
    }
}

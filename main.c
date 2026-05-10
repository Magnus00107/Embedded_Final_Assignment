/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: main.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: System startup, queue creation and task creation
*****************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "app.h"
#include "app_types.h"
#include "keypad.h"
#include "lcd.h"
#include "digiswitch.h"
#include "led.h"
#include "switches.h"
#include "loggerMain.h"

/***************************** Defines *******************************/

#define INPUT_QUEUE_LENGTH      8U
#define LCD_QUEUE_LENGTH        4U
#define LED_QUEUE_LENGTH        4U

#define DIGISWITCH_TASK_STACK_SIZE  128U
#define LCD_TASK_STACK_SIZE         256U
#define KEYPAD_TASK_STACK_SIZE      256U
#define APP_TASK_STACK_SIZE         256U
#define LED_TASK_STACK_SIZE         128U
#define SWITCH_TASK_STACK_SIZE      128U

#define LED_TASK_PRIORITY           1U
#define DIGISWITCH_TASK_PRIORITY    2U
#define LCD_TASK_PRIORITY           1U
#define KEYPAD_TASK_PRIORITY        2U
#define APP_TASK_PRIORITY           2U
#define SWITCH_TASK_PRIORITY        1U

/***************************** Local helper *******************************/

static void Error_Stop(const char *msg)
{
    UART0_SendString(msg);
    UART0_SendString("\r\n");

    while(1)
    {
    }
}

/***************************** Main *******************************/

int main(void)
{
    BaseType_t taskStatus;

    UART0_Init();
    UART0_SendString("System boot\r\n");

    xInputQueue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(InputEvent_t));
    xLcdQueue   = xQueueCreate(LCD_QUEUE_LENGTH, sizeof(LcdMessage_t));
    xLedQueue   = xQueueCreate(LED_QUEUE_LENGTH, sizeof(LedMessage_t));

    uartQueue = xQueueCreate(UART_QUEUE_LENGTH, sizeof(char[UART_MSG_SIZE]));
    logQueue  = xQueueCreate(LOG_QUEUE_LENGTH, sizeof(LogEntry));

    if(xInputQueue == NULL) Error_Stop("ERROR: xInputQueue failed");
    if(xLcdQueue   == NULL) Error_Stop("ERROR: xLcdQueue failed");
    if(xLedQueue   == NULL) Error_Stop("ERROR: xLedQueue failed");
    if(uartQueue   == NULL) Error_Stop("ERROR: uartQueue failed");
    if(logQueue    == NULL) Error_Stop("ERROR: logQueue failed");

    taskStatus = xTaskCreate(LCD_Task, "LCD", LCD_TASK_STACK_SIZE, NULL, LCD_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(Keypad_Task, "KEYPAD", KEYPAD_TASK_STACK_SIZE, NULL, KEYPAD_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(DigiSwitch_Task, "DIGISWITCH", DIGISWITCH_TASK_STACK_SIZE, NULL, DIGISWITCH_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(App_Task, "APP", APP_TASK_STACK_SIZE, NULL, APP_TASK_PRIORITY, &xAppTaskHandle);
    taskStatus = xTaskCreate(Led_Task, "LED", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(Switch_Task, "SWITCH", SWITCH_TASK_STACK_SIZE, NULL, SWITCH_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(UART_Task, "UART", UART_TASK_STACK_SIZE, NULL, UART_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(Logger_Task, "LOGGER", LOGGER_TASK_STACK_SIZE, NULL, LOGGER_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(Clock_Task, "CLOCK", CLOCK_TASK_STACK_SIZE, NULL, CLOCK_TASK_PRIORITY, NULL);
    taskStatus = xTaskCreate(Command_Task, "COMMAND", COMMAND_TASK_STACK_SIZE, NULL, COMMAND_TASK_PRIORITY, NULL);

    UART0_SendString("Starting scheduler\r\n");

    vTaskStartScheduler();

    Error_Stop("ERROR: Scheduler failed");

    while(1)
    {
    }
}

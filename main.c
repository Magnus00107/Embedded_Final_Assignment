/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: main.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: System startup, queue creation and task creation
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260429  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/

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

/*****************************    Defines    *******************************/

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

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

int main(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Creates queues, creates tasks and starts the FreeRTOS scheduler
******************************************************************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

int main(void)
{
    xInputQueue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(InputEvent_t));
    xLcdQueue   = xQueueCreate(LCD_QUEUE_LENGTH, sizeof(LcdMessage_t));
    xLedQueue   = xQueueCreate(LED_QUEUE_LENGTH, sizeof(LedMessage_t));

    if ((xInputQueue == NULL) || (xLcdQueue == NULL) || (xLedQueue == NULL))
    {
        while (1)
        {
        }
    }

    xTaskCreate(LCD_Task,
                "LCD",
                LCD_TASK_STACK_SIZE,
                NULL,
                LCD_TASK_PRIORITY,
                NULL);

    xTaskCreate(Keypad_Task,
                "KEYPAD",
                KEYPAD_TASK_STACK_SIZE,
                NULL,
                KEYPAD_TASK_PRIORITY,
                NULL);

    xTaskCreate(DigiSwitch_Task,
                "DIGISWITCH",
                DIGISWITCH_TASK_STACK_SIZE,
                NULL,
                DIGISWITCH_TASK_PRIORITY,
                NULL);

    xTaskCreate(App_Task,
                "APP",
                APP_TASK_STACK_SIZE,
                NULL,
                APP_TASK_PRIORITY,
                &xAppTaskHandle);

    xTaskCreate(Led_Task,
                "LED",
                LED_TASK_STACK_SIZE,
                NULL,
                LED_TASK_PRIORITY,
                NULL);

    xTaskCreate(Switch_Task,
                "SWITCH",
                SWITCH_TASK_STACK_SIZE,
                NULL,
                SWITCH_TASK_PRIORITY,
                NULL);

    vTaskStartScheduler();

    while (1)
    {
    }
}

/****************************** End Of Function ****************************/

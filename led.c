/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: led.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: LED driver and FreeRTOS LED task
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260430  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "led.h"
#include "app.h"
#include "app_types.h"

/*****************************    Defines    *******************************/

#define LED_RED         (1U << 1)   /* PF1 */
#define LED_YELLOW      (1U << 2)   /* PF2 */
#define LED_GREEN       (1U << 3)   /* PF3 */
#define LED_MASK        (LED_RED | LED_YELLOW | LED_GREEN)

#define LED_BLINK_HALF_PERIOD_MS    500U

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

static uint8_t Led_ColorToMask(LedColor_t color);
/*****************************************************************************
*   Input    : color - LED color enum
*   Output   : GPIO bit mask for selected LED
*   Function : Converts LED color enum to GPIO mask
******************************************************************************/

static void Led_AllOff(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Turns all LEDs off
******************************************************************************/

static void Led_Set(LedColor_t color);
/*****************************************************************************
*   Input    : color - LED color enum
*   Output   : -
*   Function : Turns on selected LED and turns off the others
******************************************************************************/

/*****************************   Functions   *******************************/

static uint8_t Led_ColorToMask(LedColor_t color)
{
    switch (color)
    {
        case LED_COLOR_RED:
            return LED_RED;

        case LED_COLOR_YELLOW:
            return LED_YELLOW;

        case LED_COLOR_GREEN:
            return LED_GREEN;

        case LED_COLOR_NONE:
        default:
            return 0U;
    }
}

/****************************** End Of Function ****************************/

static void Led_AllOff(void)
{
    GPIO_PORTF_DATA_R |= LED_MASK;
}

/****************************** End Of Function ****************************/

static void Led_Set(LedColor_t color)
{
    uint8_t mask;

    mask = Led_ColorToMask(color);

    GPIO_PORTF_DATA_R |= LED_MASK;
    GPIO_PORTF_DATA_R &= ~mask;
}

/****************************** End Of Function ****************************/

void Led_Init(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 5);     /* Enable Port F */
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTF_DIR_R |= LED_MASK;
    GPIO_PORTF_DEN_R |= LED_MASK;

    Led_AllOff();
}

/****************************** End Of Function ****************************/

void Led_Task(void *pvParameters)
{
    LedMessage_t ledMessage;
    uint32_t elapsedMs;

    (void)pvParameters;

    Led_Init();

    for (;;)
    {
        if (xQueueReceive(xLedQueue, &ledMessage, portMAX_DELAY) == pdPASS)
        {
            switch (ledMessage.mode)
            {
                case LED_MODE_OFF:
                    Led_AllOff();
                    break;

                case LED_MODE_ON_FOR_TIME:
                    Led_Set(ledMessage.color);
                    vTaskDelay(pdMS_TO_TICKS(ledMessage.duration_ms));
                    Led_AllOff();

                    if (xAppTaskHandle != NULL)
                    {
                        xTaskNotifyGive(xAppTaskHandle);
                    }
                    break;

                case LED_MODE_BLINK:
                    elapsedMs = 0U;

                    while (elapsedMs < ledMessage.duration_ms)
                    {
                        Led_Set(ledMessage.color);
                        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_HALF_PERIOD_MS));
                        elapsedMs += LED_BLINK_HALF_PERIOD_MS;

                        if (elapsedMs >= ledMessage.duration_ms)
                        {
                            break;
                        }

                        Led_AllOff();
                        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_HALF_PERIOD_MS));
                        elapsedMs += LED_BLINK_HALF_PERIOD_MS;
                    }

                    Led_AllOff();
                    if (xAppTaskHandle != NULL)
                    {
                        xTaskNotifyGive(xAppTaskHandle);
                    }
                    break;
            }
        }
    }
}

/****************************** End Of Function ****************************/

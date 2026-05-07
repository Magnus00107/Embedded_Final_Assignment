/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: digiswitch.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Digital switch / rotary encoder driver and FreeRTOS task
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
#include "digiswitch.h"
#include "app_types.h"
#include "app.h"

/*****************************    Defines    *******************************/

#define DIGI_A                  (1U << 5)   /* PA5 */
#define DIGI_B                  (1U << 6)   /* PA6 */
#define DIGI_P2                 (1U << 7)   /* PA7 */

#define DIGI_SCAN_DELAY_MS      5U

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

static uint8_t DigiSwitch_ReadA(void);
/*****************************************************************************
*   Input    : -
*   Output   : Current logic level on DIGI A
*   Function : Reads channel A input
******************************************************************************/

static uint8_t DigiSwitch_ReadB(void);
/*****************************************************************************
*   Input    : -
*   Output   : Current logic level on DIGI B
*   Function : Reads channel B input
******************************************************************************/

static uint8_t DigiSwitch_ReadButton(void);
/*****************************************************************************
*   Input    : -
*   Output   : Current logic level on DIGI P2
*   Function : Reads digital switch push button input
******************************************************************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

static uint8_t DigiSwitch_ReadA(void)
{
    return ((GPIO_PORTA_DATA_R & DIGI_A) ? 1U : 0U);
}

/****************************** End Of Function ****************************/

static uint8_t DigiSwitch_ReadB(void)
{
    return ((GPIO_PORTA_DATA_R & DIGI_B) ? 1U : 0U);
}

/****************************** End Of Function ****************************/

static uint8_t DigiSwitch_ReadButton(void)
{
    return ((GPIO_PORTA_DATA_R & DIGI_P2) ? 1U : 0U);
}

/****************************** End Of Function ****************************/

void DigiSwitch_Init(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 0);        /* Enable Port A */
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTA_DIR_R &= ~(DIGI_A | DIGI_B | DIGI_P2);
    GPIO_PORTA_DEN_R |=  (DIGI_A | DIGI_B | DIGI_P2);
}

/****************************** End Of Function ****************************/

void DigiSwitch_Task(void *pvParameters)
{
    uint8_t prevA;
    uint8_t prevButton;
    InputEvent_t inputEvent;

    (void)pvParameters;

    DigiSwitch_Init();

    prevA = DigiSwitch_ReadA();
    prevButton = DigiSwitch_ReadButton();

    for (;;)
    {
        uint8_t currentA = DigiSwitch_ReadA();
        uint8_t currentB = DigiSwitch_ReadB();
        uint8_t currentButton = DigiSwitch_ReadButton();

        /* Detect rotation on rising edge of channel A */
        if (currentA != prevA)
        {
            inputEvent.key = 0;

            if (currentA == currentB)
            {
                inputEvent.type = INPUT_EVENT_DIGI_LEFT;
            }
            else
            {
                inputEvent.type = INPUT_EVENT_DIGI_RIGHT;
            }

            xQueueSend(xInputQueue, &inputEvent, 0);
        }

        if ((prevButton == 0U) && (currentButton == 1U))
        {
            inputEvent.type = INPUT_EVENT_DIGI_BUTTON;
            inputEvent.key = 0;
            xQueueSend(xInputQueue, &inputEvent, 0);
        }

        prevA = currentA;
        prevButton = currentButton;

        vTaskDelay(pdMS_TO_TICKS(DIGI_SCAN_DELAY_MS));
    }
}
/****************************** End Of Function ****************************/

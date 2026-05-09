/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: digiswitch.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Digital switch / rotary encoder driver and FreeRTOS task.
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

#define DIGI_SWITCH_A_PIN       (1U << 5)   /* PA5 */
#define DIGI_SWITCH_B_PIN       (1U << 6)   /* PA6 */
#define DIGI_SWITCH_BUTTON_PIN  (1U << 7)   /* PA7 */

#define DIGI_SWITCH_PORT_A      (1U << 0)
#define DIGI_SWITCH_SCAN_TIME   5U
#define DIGI_SWITCH_LOW         0U
#define DIGI_SWITCH_HIGH        1U

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

static uint8_t digi_switch_read_pin(uint32_t pin_mask)
/*****************************************************************************
*   Input    : Pin mask for the selected GPIO input
*   Output   : Logic level of the selected input pin
*   Function : Reads one digital switch input from GPIO port A
******************************************************************************/
{
    uint8_t pin_state = DIGI_SWITCH_LOW;

    if ((GPIO_PORTA_DATA_R & pin_mask) != 0U)
    {
        pin_state = DIGI_SWITCH_HIGH;
    }

    return pin_state;
}

/****************************** End Of Function ****************************/

static void digi_switch_send_event(InputEventType_t event_type)
/*****************************************************************************
*   Input    : Event type to send
*   Output   : -
*   Function : Sends a digital switch input event to the input queue
******************************************************************************/
{
    InputEvent_t input_event;

    input_event.type = event_type;
    input_event.key = 0U;

    xQueueSend(xInputQueue, &input_event, 0U);
}

/****************************** End Of Function ****************************/

void DigiSwitch_Init(void)
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO port A pins used by the digital switch
******************************************************************************/
{
    volatile uint32_t dummy_read;

    SYSCTL_RCGCGPIO_R |= DIGI_SWITCH_PORT_A;
    dummy_read = SYSCTL_RCGCGPIO_R;
    (void)dummy_read;

    GPIO_PORTA_DIR_R &= ~(DIGI_SWITCH_A_PIN |
                          DIGI_SWITCH_B_PIN |
                          DIGI_SWITCH_BUTTON_PIN);

    GPIO_PORTA_DEN_R |=  (DIGI_SWITCH_A_PIN |
                          DIGI_SWITCH_B_PIN |
                          DIGI_SWITCH_BUTTON_PIN);
}

/****************************** End Of Function ****************************/

void DigiSwitch_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Polls the digital switch and sends rotation/button events
******************************************************************************/
{
    uint8_t previous_a_state;
    uint8_t previous_button_state;

    (void)pvParameters;

    DigiSwitch_Init();

    previous_a_state = digi_switch_read_pin(DIGI_SWITCH_A_PIN);
    previous_button_state = digi_switch_read_pin(DIGI_SWITCH_BUTTON_PIN);

    for (;;)
    {
        uint8_t current_a_state;
        uint8_t current_b_state;
        uint8_t current_button_state;

        current_a_state = digi_switch_read_pin(DIGI_SWITCH_A_PIN);
        current_b_state = digi_switch_read_pin(DIGI_SWITCH_B_PIN);
        current_button_state = digi_switch_read_pin(DIGI_SWITCH_BUTTON_PIN);

        if (current_a_state != previous_a_state)
        {
            if (current_a_state == current_b_state)
            {
                digi_switch_send_event(INPUT_EVENT_DIGI_LEFT);
            }
            else
            {
                digi_switch_send_event(INPUT_EVENT_DIGI_RIGHT);
            }
        }

        if ((previous_button_state == DIGI_SWITCH_LOW) &&
            (current_button_state == DIGI_SWITCH_HIGH))
        {
            digi_switch_send_event(INPUT_EVENT_DIGI_BUTTON);
        }

        previous_a_state = current_a_state;
        previous_button_state = current_button_state;

        vTaskDelay(pdMS_TO_TICKS(DIGI_SWITCH_SCAN_TIME));
    }
}

/****************************** End Of Function ****************************/

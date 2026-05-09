/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: switches.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Switch driver and FreeRTOS task for SW1 and SW2.
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260503  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "switches.h"
#include "app_types.h"
#include "app.h"

/*****************************    Defines    *******************************/

#define SWITCH_1_PIN            (1U << 4)   /* PF4 */
#define SWITCH_2_PIN            (1U << 0)   /* PF0 */

#define SWITCH_PORT_F           (1U << 5)
#define SWITCH_UNLOCK_VALUE     0x4C4F434B
#define SWITCH_SCAN_TIME_MS     10U

#define SWITCH_RELEASED         1U
#define SWITCH_PRESSED          0U

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

static uint8_t switch_read_pin(uint32_t pin_mask)
/*****************************************************************************
*   Input    : Pin mask for the selected GPIO input
*   Output   : Current logic level of the selected switch pin
*   Function : Reads one switch input from GPIO port F
******************************************************************************/
{
    uint8_t pin_state = SWITCH_PRESSED;

    if ((GPIO_PORTF_DATA_R & pin_mask) != 0U)
    {
        pin_state = SWITCH_RELEASED;
    }

    return pin_state;
}

/****************************** End Of Function ****************************/

static void switch_send_event(InputEventType_t event_type)
/*****************************************************************************
*   Input    : Event type to send
*   Output   : -
*   Function : Sends a switch event to the input queue
******************************************************************************/
{
    InputEvent_t input_event;

    input_event.type = event_type;
    input_event.key = 0U;

    xQueueSend(xInputQueue, &input_event, 0U);
}

/****************************** End Of Function ****************************/

void Switch_Init(void)
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO port F pins used by SW1 and SW2
******************************************************************************/
{
    volatile uint32_t dummy_read;

    SYSCTL_RCGCGPIO_R |= SWITCH_PORT_F;
    dummy_read = SYSCTL_RCGCGPIO_R;
    (void)dummy_read;

    GPIO_PORTF_LOCK_R = SWITCH_UNLOCK_VALUE;
    GPIO_PORTF_CR_R |= SWITCH_2_PIN;

    GPIO_PORTF_DIR_R &= ~(SWITCH_1_PIN | SWITCH_2_PIN);
    GPIO_PORTF_DEN_R |=  (SWITCH_1_PIN | SWITCH_2_PIN);
    GPIO_PORTF_PUR_R |=  (SWITCH_1_PIN | SWITCH_2_PIN);
}

/****************************** End Of Function ****************************/

void Switch_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Polls SW1 and SW2 and sends switch input events
******************************************************************************/
{
    uint8_t previous_switch_1_state;
    uint8_t previous_switch_2_state;
    uint8_t current_switch_1_state;
    uint8_t current_switch_2_state;

    (void)pvParameters;

    Switch_Init();

    previous_switch_1_state = switch_read_pin(SWITCH_1_PIN);
    previous_switch_2_state = switch_read_pin(SWITCH_2_PIN);

    while (1)
    {
        current_switch_1_state = switch_read_pin(SWITCH_1_PIN);
        current_switch_2_state = switch_read_pin(SWITCH_2_PIN);

        if ((previous_switch_1_state == SWITCH_RELEASED) &&
            (current_switch_1_state == SWITCH_PRESSED))
        {
            switch_send_event(INPUT_EVENT_SW1);
        }

        if ((previous_switch_2_state == SWITCH_RELEASED) &&
            (current_switch_2_state == SWITCH_PRESSED))
        {
            switch_send_event(INPUT_EVENT_SW2_PRESSED);
        }

        if ((previous_switch_2_state == SWITCH_PRESSED) &&
            (current_switch_2_state == SWITCH_RELEASED))
        {
            switch_send_event(INPUT_EVENT_SW2_RELEASED);
        }

        previous_switch_1_state = current_switch_1_state;
        previous_switch_2_state = current_switch_2_state;

        vTaskDelay(pdMS_TO_TICKS(SWITCH_SCAN_TIME_MS));
    }
}

/****************************** End Of Function ****************************/

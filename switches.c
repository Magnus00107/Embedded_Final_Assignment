#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "switches.h"
#include "app_types.h"
#include "app.h"

#define SW1     (1U << 4)   /* PF4 */
#define SW2     (1U << 0)   /* PF0 */

#define DELAY_MS  10U



void Switch_Init(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 5);     /* port F clock */
    dummy = SYSCTL_RCGCGPIO_R;

    /* unlock PF0 */
    GPIO_PORTF_LOCK_R = 0x4C4F434B;
    GPIO_PORTF_CR_R |= SW2;

    GPIO_PORTF_DIR_R &= ~(SW1 | SW2);   /* input */
    GPIO_PORTF_DEN_R |=  (SW1 | SW2);   /* digital enable */
    GPIO_PORTF_PUR_R |=  (SW1 | SW2);   /* pull-up */

}

void Switch_Task(void *pvParameters)
{
    uint8_t old_sw1;
    uint8_t old_sw2;
    uint8_t sw1;
    uint8_t sw2;

    InputEvent_t event;

    (void)pvParameters;

    Switch_Init();

    old_sw1 = GPIO_PORTF_DATA_R & SW1;
    old_sw2 = GPIO_PORTF_DATA_R & SW2;

    while (1)
    {
        sw1 = GPIO_PORTF_DATA_R & SW1;
        sw2 = GPIO_PORTF_DATA_R & SW2;

        /* switches are active low */
        if ((old_sw1 != 0U) && (sw1 == 0U))
        {
            event.type = INPUT_EVENT_SW1;
            event.key = 0;
            xQueueSend(xInputQueue, &event, 0);
        }

        if ((old_sw2 != 0U) && (sw2 == 0U))
        {
            event.type = INPUT_EVENT_SW2_PRESSED;
            event.key = 0;
            xQueueSend(xInputQueue, &event, 0);
        }

        if ((old_sw2 == 0U) && (sw2 != 0U))
        {
            event.type = INPUT_EVENT_SW2_RELEASED;
            event.key = 0;
            xQueueSend(xInputQueue, &event, 0);
        }

        old_sw1 = sw1;
        old_sw2 = sw2;

        vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
    }
}



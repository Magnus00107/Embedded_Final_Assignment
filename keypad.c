/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: keypad.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Matrix keypad driver and FreeRTOS keypad task
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

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "keypad.h"
#include "app_types.h"
#include "app.h"

/*****************************    Defines    *******************************/

/* Columns: PA2, PA3, PA4 */
#define COL1                (1U << 2)
#define COL2                (1U << 3)
#define COL3                (1U << 4)
#define COL_MASK            (COL1 | COL2 | COL3)

/* Rows: PE0, PE1, PE2, PE3 */
#define ROW1                (1U << 0)
#define ROW2                (1U << 1)
#define ROW3                (1U << 2)
#define ROW4                (1U << 3)
#define ROW_MASK            (ROW1 | ROW2 | ROW3 | ROW4)

#define KEYPAD_SCAN_DELAY_MS    50U

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

static void Keypad_Delay(volatile uint32_t delay);
/*****************************************************************************
*   Input    : delay - simple loop count
*   Output   : -
*   Function : Small blocking delay used for keypad signal settling
******************************************************************************/

static char Keypad_Decode(uint8_t rowIndex, uint8_t colIndex);
/*****************************************************************************
*   Input    : rowIndex - row number 0..3
*              colIndex - column number 0..2
*   Output   : Corresponding keypad character
*   Function : Converts row/column position to keypad character
******************************************************************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

static void Keypad_Delay(volatile uint32_t delay)
{
    while (delay--)
    {
    }
}

/****************************** End Of Function ****************************/

static char Keypad_Decode(uint8_t rowIndex, uint8_t colIndex)
{
    static const char keymap[4][3] =
    {
        {'#', '0', '*'},
        {'9', '8', '7'},
        {'6', '5', '4'},
        {'3', '2', '1'}
    };

    return keymap[rowIndex][colIndex];
}

/****************************** End Of Function ****************************/

void Keypad_Init(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 0) | (1U << 4);   /* Enable Port A and E */
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    /* Configure columns as outputs */
    GPIO_PORTA_DIR_R |= COL_MASK;
    GPIO_PORTA_DEN_R |= COL_MASK;
    GPIO_PORTA_DATA_R &= ~COL_MASK;

    /* Configure rows as inputs */
    GPIO_PORTE_DIR_R &= ~ROW_MASK;
    GPIO_PORTE_DEN_R |= ROW_MASK;
}

/****************************** End Of Function ****************************/

char Keypad_Scan(void)
{
    uint8_t colIndex;
    uint32_t rowValue;
    const uint8_t cols[3] = {COL1, COL2, COL3};

    for (colIndex = 0; colIndex < 3; colIndex++)
    {
        GPIO_PORTA_DATA_R &= ~COL_MASK;
        GPIO_PORTA_DATA_R |= cols[colIndex];

        Keypad_Delay(2000);

        rowValue = GPIO_PORTE_DATA_R & ROW_MASK;

        if (rowValue & ROW1)
            return Keypad_Decode(0, colIndex);

        if (rowValue & ROW2)
            return Keypad_Decode(1, colIndex);

        if (rowValue & ROW3)
            return Keypad_Decode(2, colIndex);

        if (rowValue & ROW4)
            return Keypad_Decode(3, colIndex);
    }

    return 0;
}

/****************************** End Of Function ****************************/

void Keypad_Task(void *pvParameters)
{
    char currentKey;
    char previousKey = 0;
    InputEvent_t inputEvent;

    (void)pvParameters;

    Keypad_Init();

    for (;;)
    {
        currentKey = Keypad_Scan();

        if ((currentKey != 0) && (previousKey == 0))
        {
            inputEvent.type = INPUT_EVENT_KEYPAD;
            inputEvent.key  = currentKey;
            xQueueSend(xInputQueue, &inputEvent, 0);
        }

        previousKey = currentKey;

        vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_DELAY_MS));
    }
}

/****************************** End Of Function ****************************/

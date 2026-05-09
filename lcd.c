/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: lcd.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: HD44780 LCD driver and FreeRTOS LCD task
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
#include <string.h>
#include "tm4c123gh6pm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "app.h"
#include "app_types.h"

/*****************************    Defines    *******************************/

#define LCD_RS              (1U << 2)   /* PD2 */
#define LCD_E               (1U << 3)   /* PD3 */

#define LCD_D4              (1U << 4)   /* PC4 */
#define LCD_D5              (1U << 5)   /* PC5 */
#define LCD_D6              (1U << 6)   /* PC6 */
#define LCD_D7              (1U << 7)   /* PC7 */

#define LCD_DATA_MASK       (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

static void LCD_Delay(volatile uint32_t delay);
/*****************************************************************************
*   Input    : delay - simple loop count
*   Output   : -
*   Function : Small blocking delay used for LCD timing
******************************************************************************/

static void LCD_PulseEnable(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Generates enable pulse on LCD E pin
******************************************************************************/

static void LCD_WriteNibble(uint8_t nibble);
/*****************************************************************************
*   Input    : nibble - lower 4 bits to write to LCD data pins
*   Output   : -
*   Function : Writes one 4-bit nibble to the LCD
******************************************************************************/

static void LCD_WriteByte(uint8_t rs, uint8_t value);
/*****************************************************************************
*   Input    : rs    - 0 for command, 1 for data
*              value - byte to send
*   Output   : -
*   Function : Writes one byte to the LCD in 4-bit mode
******************************************************************************/

static void LCD_Command(uint8_t command);
/*****************************************************************************
*   Input    : command - LCD command byte
*   Output   : -
*   Function : Sends command to LCD
******************************************************************************/

static void LCD_Data(uint8_t data);
/*****************************************************************************
*   Input    : data - character byte
*   Output   : -
*   Function : Sends character data to LCD
******************************************************************************/

static void LCD_InitGPIO(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO ports and pins used by LCD
******************************************************************************/

static void LCD_Clear(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Clears LCD display
******************************************************************************/

static void LCD_SetCursor(uint8_t row, uint8_t col);
/*****************************************************************************
*   Input    : row - LCD row number
*              col - LCD column number
*   Output   : -
*   Function : Sets LCD cursor position
******************************************************************************/

static void LCD_WriteLine(uint8_t row, const char *text);
/*****************************************************************************
*   Input    : row  - LCD row number
*              text - string to display
*   Output   : -
*   Function : Writes one full LCD line and pads remaining positions with spaces
******************************************************************************/

/*****************************   Variables   *******************************/

/*****************************   Functions   *******************************/

static void LCD_Delay(volatile uint32_t delay)
{
    while (delay--)
    {
    }
}

/****************************** End Of Function ****************************/

static void LCD_PulseEnable(void)
{
    GPIO_PORTD_DATA_R |= LCD_E;
    LCD_Delay(2000);
    GPIO_PORTD_DATA_R &= ~LCD_E;
    LCD_Delay(2000);
}

/****************************** End Of Function ****************************/

static void LCD_WriteNibble(uint8_t nibble)
{
    GPIO_PORTC_DATA_R &= ~LCD_DATA_MASK;

    if (nibble & 0x01)
        GPIO_PORTC_DATA_R |= LCD_D4;

    if (nibble & 0x02)
        GPIO_PORTC_DATA_R |= LCD_D5;

    if (nibble & 0x04)
        GPIO_PORTC_DATA_R |= LCD_D6;

    if (nibble & 0x08)
        GPIO_PORTC_DATA_R |= LCD_D7;

    LCD_PulseEnable();
}

/****************************** End Of Function ****************************/

static void LCD_WriteByte(uint8_t rs, uint8_t value)
{
    if (rs != 0)
        GPIO_PORTD_DATA_R |= LCD_RS;
    else
        GPIO_PORTD_DATA_R &= ~LCD_RS;

    LCD_WriteNibble((value >> 4) & 0x0F);
    LCD_WriteNibble(value & 0x0F);

    LCD_Delay(10000);
}

/****************************** End Of Function ****************************/

static void LCD_Command(uint8_t command)
{
    LCD_WriteByte(0, command);
}

/****************************** End Of Function ****************************/

static void LCD_Data(uint8_t data)
{
    LCD_WriteByte(1, data);
}

/****************************** End Of Function ****************************/

static void LCD_InitGPIO(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 2) | (1U << 3);   /* Enable Port C and D */
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTC_DIR_R |= LCD_DATA_MASK;
    GPIO_PORTC_DEN_R |= LCD_DATA_MASK;

    GPIO_PORTD_DIR_R |= (LCD_RS | LCD_E);
    GPIO_PORTD_DEN_R |= (LCD_RS | LCD_E);

    GPIO_PORTC_DATA_R &= ~LCD_DATA_MASK;
    GPIO_PORTD_DATA_R &= ~(LCD_RS | LCD_E);
}

/****************************** End Of Function ****************************/

void LCD_Init(void)
{
    LCD_InitGPIO();

    LCD_Delay(200000);

    GPIO_PORTD_DATA_R &= ~LCD_RS;

    LCD_WriteNibble(0x03);
    LCD_Delay(80000);

    LCD_WriteNibble(0x03);
    LCD_Delay(20000);

    LCD_WriteNibble(0x03);
    LCD_Delay(20000);

    LCD_WriteNibble(0x02);
    LCD_Delay(20000);

    LCD_Command(0x28);      /* 4-bit, 2-line, 5x8 */
    LCD_Command(0x08);      /* display off */
    LCD_Command(0x01);      /* clear display */
    LCD_Delay(80000);
    LCD_Command(0x06);      /* entry mode set */
    LCD_Command(0x0C);      /* display on, cursor off, blink off */
}

/****************************** End Of Function ****************************/

static void LCD_Clear(void)
{
    LCD_Command(0x01);
    LCD_Delay(80000);
}

/****************************** End Of Function ****************************/

static void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address;

    if (row == 0)
        address = 0x80 + col;
    else
        address = 0xC0 + col;

    LCD_Command(address);
}

/****************************** End Of Function ****************************/

static void LCD_WriteLine(uint8_t row, const char *text)
{
    uint8_t i;

    LCD_SetCursor(row, 0);

    for (i = 0; i < 16; i++)
    {
        if (text[i] == '\0')
            break;

        LCD_Data((uint8_t)text[i]);
    }

    for (; i < 16; i++)
    {
        LCD_Data(' ');
    }
}

/****************************** End Of Function ****************************/

void LCD_Task(void *pvParameters)
{
    LcdMessage_t lcdMessage;

    (void)pvParameters;

    LCD_Init();
    LCD_Clear();
    LCD_WriteLine(0, "System starting");
    LCD_WriteLine(1, "");

    for (;;)
    {
        if (xQueueReceive(xLcdQueue, &lcdMessage, portMAX_DELAY) == pdPASS)
        {
            LCD_Clear();
            LCD_WriteLine(0, lcdMessage.line1);
            LCD_WriteLine(1, lcdMessage.line2);
        }
    }
}

/****************************** End Of Function ****************************/

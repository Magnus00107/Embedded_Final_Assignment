/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: lcd.h
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: LCD driver interface and FreeRTOS LCD task declarations
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260429  MoH   Module created.
*
*****************************************************************************/

#ifndef LCD_H
#define LCD_H

/***************************** Include files *******************************/

#include <stdint.h>

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

void LCD_Init(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO and LCD controller
******************************************************************************/

void LCD_Task(void *pvParameters);
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : FreeRTOS task that receives display messages from queue and
*              updates the LCD
******************************************************************************/

/****************************** End Of Module *******************************/
#endif /* LCD_H */

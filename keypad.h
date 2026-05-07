/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: keypad.h
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Keypad driver interface and keypad task declarations
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260429  MoH   Module created.
*
*****************************************************************************/

#ifndef KEYPAD_H
#define KEYPAD_H

/***************************** Include files *******************************/

#include "FreeRTOS.h"
#include "queue.h"

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

void Keypad_Init(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO used for keypad scanning
******************************************************************************/

char Keypad_Scan(void);
/*****************************************************************************
*   Input    : -
*   Output   : Pressed key as character, 0 if no key is pressed
*   Function : Scans keypad matrix once and returns detected key
******************************************************************************/

void Keypad_Task(void *pvParameters);
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : FreeRTOS task that scans keypad and sends key events to queue
******************************************************************************/

/****************************** End Of Module *******************************/
#endif /* KEYPAD_H */

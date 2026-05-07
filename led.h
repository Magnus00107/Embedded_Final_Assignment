/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: led.h
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: LED driver interface and LED task declarations
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260430  MoH   Module created.
*
*****************************************************************************/

#ifndef LED_H
#define LED_H

/***************************** Include files *******************************/

#include "FreeRTOS.h"
#include "queue.h"

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

void Led_Init(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO used for onboard LEDs
******************************************************************************/

void Led_Task(void *pvParameters);
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : FreeRTOS task that controls LED behavior from queue messages
******************************************************************************/

/****************************** End Of Module *******************************/
#endif /* LED_H */

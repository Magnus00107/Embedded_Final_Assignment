/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: digiswitch.h
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Digital switch / rotary encoder driver interface
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260430  MoH   Module created.
*
*****************************************************************************/

#ifndef DIGISWITCH_H
#define DIGISWITCH_H

/***************************** Include files *******************************/

#include "FreeRTOS.h"
#include "queue.h"

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

void DigiSwitch_Init(void);
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes GPIO used for digital switch input
******************************************************************************/

void DigiSwitch_Task(void *pvParameters);
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : FreeRTOS task that detects rotation and button press events
******************************************************************************/

/****************************** End Of Module *******************************/
#endif /* DIGISWITCH_H */

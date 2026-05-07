/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: app.h
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Application task declarations and shared queue handles
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260429  MoH   Module created.
*
*****************************************************************************/

#ifndef APP_H
#define APP_H

/***************************** Include files *******************************/

#include "FreeRTOS.h"
#include "queue.h"

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

extern QueueHandle_t xInputQueue;
extern QueueHandle_t xLcdQueue;
extern QueueHandle_t xLedQueue;
extern TaskHandle_t xAppTaskHandle;

void App_Task(void *pvParameters);
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : Main application state machine task
******************************************************************************/

/****************************** End Of Module *******************************/
#endif /* APP_H */

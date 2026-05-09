/*****************************************************************************
 * University of Southern Denmark
 * Embedded C Programming (ECP)
 *
 * MODULENAME.: app_types.h
 *
 * PROJECT....: Poster Assignment
 *
 * DESCRIPTION: Shared application type definitions
 *
 * Change Log:
 ******************************************************************************
 * Date    Id    Change
 * YYMMDD
 * --------------------
 * 260429  MoH   Module created.
 *
 *****************************************************************************/

#ifndef APP_TYPES_H
#define APP_TYPES_H

/***************************** Include files *******************************/

#include <stdint.h>

/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Functions   *******************************/

/**************************   Type definitions   ***************************/

typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_SELECT_PRODUCT,
    APP_STATE_SELECT_PAYMENT,
    APP_STATE_TRANSACTION,
    APP_STATE_CASH,
    APP_STATE_CARD,
    APP_STATE_PAYBACK,
    APP_STATE_PRODUCTION,
    APP_STATE_DONE

} AppState_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Main application states
 ******************************************************************************/

typedef enum
{
    PAYMENT_NONE = 0,
    PAYMENT_CASH,
    PAYMENT_CARD
} Payment_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Payment selection values
 ******************************************************************************/

typedef enum
{
    PAYMENT_CARD_NONE = 0,
    PAYMENT_CARD_NUMBER,
    PAYMENT_CARD_PIN
}   PaymentCard_t;

typedef struct
{
    uint16_t espresso_cup_dkk;  //u = unsigned, t_ = type
    uint16_t latte_cup_dkk;
    uint16_t filter_dkk_per_cl;
} ProductPrices_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Holds configurable product prices
 ******************************************************************************/

typedef enum
{
    PRODUCT_NONE = 0,
    PRODUCT_ESPRESSO,
    PRODUCT_LATTE,
    PRODUCT_FILTER
} Product_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Product selection values
 ******************************************************************************/

typedef enum
{
    INPUT_EVENT_NONE = 0,
    INPUT_EVENT_KEYPAD,
    INPUT_EVENT_DIGI_LEFT,
    INPUT_EVENT_DIGI_RIGHT,
    INPUT_EVENT_DIGI_BUTTON,
    INPUT_EVENT_SW1,
    INPUT_EVENT_SW2_PRESSED,
    INPUT_EVENT_SW2_RELEASED
} InputEventType_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Identifies source and type of user input event
 ******************************************************************************/

typedef enum
{
    LED_COLOR_NONE = 0, LED_COLOR_RED, LED_COLOR_YELLOW, LED_COLOR_GREEN
} LedColor_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : LED color selection values
 ******************************************************************************/

typedef enum
{
    LED_MODE_OFF = 0,
    LED_MODE_ON,
    LED_MODE_ON_FOR_TIME,
    LED_MODE_BLINK
} LedMode_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : LED operating modes
 ******************************************************************************/

typedef struct
{
    LedColor_t color;
    LedMode_t mode;
    uint32_t duration_ms;
} LedMessage_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Message sent from application task to LED task
 ******************************************************************************/

typedef struct
{
    InputEventType_t type;
    char key;
} InputEvent_t;
/*****************************************************************************
 *   Input    : -
 *   Output   : -
 *   Function : Message sent from input tasks to application task
 ******************************************************************************/

typedef struct
{
    char line1[17];
    char line2[17];
} LcdMessage_t;

/****************************** End Of Module *******************************/
#endif /*APP_TYPES_H_*/

/*****************************************************************************

* University of Southern Denmark

* Embedded C Programming (ECP)

*

* MODULENAME.: app.c

*

* PROJECT....: Poster Assignment

*

* DESCRIPTION: Main application state machine for coffee machine demo

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

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "app.h"
#include "app_types.h"
#include <stdio.h>
/*****************************    Defines    *******************************/

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

QueueHandle_t xInputQueue   = NULL;
QueueHandle_t xLcdQueue     = NULL;
QueueHandle_t xLedQueue     = NULL;
TaskHandle_t xAppTaskHandle = NULL;
uint8_t cupPlaced = 0;
/*****************************   Functions   *******************************/

static void App_SendDisplay(const char *line1, const char *line2)
/*****************************************************************************
*   Input    : line1 - text for LCD line 1
*              line2 - text for LCD line 2
*   Output   : -
*   Function : Sends a complete two-line LCD message to the LCD task queue
******************************************************************************/
{
    LcdMessage_t lcdMessage;

    memset(&lcdMessage, 0, sizeof(LcdMessage_t));

    strncpy(lcdMessage.line1, line1, 16);
    strncpy(lcdMessage.line2, line2, 16);

    lcdMessage.line1[16] = '\0';
    lcdMessage.line2[16] = '\0';

    xQueueSend(xLcdQueue, &lcdMessage, portMAX_DELAY);
}

static const char *App_ProductToText(Product_t product)
/*****************************************************************************
*   Input    : product - selected product enum value
*   Output   : Pointer to product text
*   Function : Converts selected product to text for display
******************************************************************************/
{
    switch (product)
    {
    case PRODUCT_ESPRESSO:
        return "Espresso";

    case PRODUCT_LATTE:
        return "Latte";

    case PRODUCT_FILTER:
        return "Filter";

    case PRODUCT_NONE:
    default:
        return "None";
    }
}

static uint16_t App_GetProductPrice(Product_t product,
                                    ProductPrices_t *prices,
                                    uint16_t filterAmountCl)
{
    switch (product)
    {
    case PRODUCT_ESPRESSO:
        return prices ->espresso_cup_dkk;

    case PRODUCT_LATTE:
        return prices ->latte_cup_dkk;

    case PRODUCT_FILTER:
        return (uint16_t)(filterAmountCl*prices->filter_dkk_per_cl);

    case PRODUCT_NONE:
    default:
        return 0;
    }
}

static void App_SendLed(LedColor_t color, LedMode_t mode, uint32_t durationMs)
{
    LedMessage_t ledMessage;

    ledMessage.color = color;
    ledMessage.mode = mode;
    ledMessage.duration_ms = durationMs;

    xQueueSend(xLedQueue, &ledMessage, portMAX_DELAY);
}

static uint8_t App_IsDigitOdd(char digit)
/*****************************************************************************
*   Input    : digit - ASCII digit character
*   Output   : 1 if odd, 0 if even
*   Function : Checks whether a digit character represents an odd number
******************************************************************************/
{
    return ((digit - '0') % 2) ? 1 : 0;
}

void App_Task(void *pvParameters)
/*****************************************************************************
*   Input    : pvParameters - not used
*   Output   : -
*   Function : Minimum application task for first LCD test
******************************************************************************/
{
    uint16_t filterAmountCl;
    uint16_t requiredPrice;
    AppState_t state = APP_STATE_IDLE;
    AppState_t previousState;
    char cardNumber[17] = "";
    char pinCode[5] = "";
    uint8_t numberIndex = 0;
    uint8_t pinIndex = 0;
    PaymentCard_t cardSequence = PAYMENT_CARD_NONE;
    Product_t selectedProduct = PRODUCT_NONE;
    Payment_t selectedPayment = PAYMENT_NONE;
    ProductPrices_t prices = {15, 27, 3};
    InputEvent_t inputEvent;
    uint16_t userWallet = 0;
    char line2[17];

    (void)pvParameters;

    App_SendDisplay("Coffee Machine", "Press #");

    for (;;)
    {
        if (xQueueReceive(xInputQueue, &inputEvent, portMAX_DELAY) == pdPASS)
        {
            do
            {
                previousState = state;

                switch (state)
                {
                    case APP_STATE_IDLE:
                        App_SendDisplay("Coffee Machine", "Press #");
                        if ((inputEvent.type == INPUT_EVENT_KEYPAD) && (inputEvent.key == '#'))
                        {
                            state = APP_STATE_SELECT_PRODUCT;
                        }
                        break;

                    case APP_STATE_SELECT_PRODUCT:
                        App_SendDisplay("1:E 2:L 3:F", "Select product");
                        if (inputEvent.type == INPUT_EVENT_KEYPAD)
                        {
                            if (inputEvent.key == '1')
                            {
                                selectedProduct = PRODUCT_ESPRESSO;
                                state = APP_STATE_SELECT_PAYMENT;
                                //App_SendDisplay(App_ProductToText(selectedProduct), "1:Cash 2:Card");

                                inputEvent.type = INPUT_EVENT_NONE;
                                inputEvent.key = 0;
                            }
                            else if (inputEvent.key == '2')
                            {
                                selectedProduct = PRODUCT_LATTE;
                                state = APP_STATE_SELECT_PAYMENT;
                                //App_SendDisplay(App_ProductToText(selectedProduct), "1:Cash 2:Card");

                                inputEvent.type = INPUT_EVENT_NONE;
                                inputEvent.key = 0;
                            }
                            else if (inputEvent.key == '3')
                            {
                                selectedProduct = PRODUCT_FILTER;
                                state = APP_STATE_SELECT_PAYMENT;

                                inputEvent.type = INPUT_EVENT_NONE;
                                inputEvent.key = 0;
                            }
                            else if (inputEvent.key == '4')
                            {
                                App_SendDisplay("Magnus er", "tyk");
                                state = APP_STATE_IDLE;

                            }
                        }
                        break;

                    case APP_STATE_SELECT_PAYMENT:
                        App_SendDisplay(App_ProductToText(selectedProduct), "1:Cash 2:Card");
                        if (inputEvent.type == INPUT_EVENT_KEYPAD)
                        {
                            if (inputEvent.key == '1')
                            {
                                selectedPayment = PAYMENT_CASH;
                                userWallet = 0;
                                state = APP_STATE_TRANSACTION;
                                //vTaskDelay(pdMS_TO_TICKS(500));

                            }
                            else if (inputEvent.key == '2')
                            {
                                selectedPayment = PAYMENT_CARD;
                                cardSequence = PAYMENT_CARD_NUMBER;
                                state = APP_STATE_CARD;
                                App_SendDisplay("Enter card no.", cardNumber);                                inputEvent.type = INPUT_EVENT_NONE;
                                inputEvent.key = 0;

                            }

                        }
                        break;

                    case APP_STATE_TRANSACTION:
                        requiredPrice = App_GetProductPrice(selectedProduct, &prices, filterAmountCl);
                        if (selectedPayment == PAYMENT_CASH)
                        {
                            if (userWallet == 0)
                            {
                                App_SendDisplay(App_ProductToText(selectedProduct), "Insert coins");

                            }


                            if (inputEvent.type == (INPUT_EVENT_DIGI_LEFT)) //|| (INPUT_EVENT_KEYPAD))
                            {
                                userWallet += 5;
                                snprintf(line2, sizeof(line2), "Paid: %u DKK", userWallet);
                                App_SendDisplay(App_ProductToText(selectedProduct), line2);
                                if ((selectedProduct == PRODUCT_FILTER) && (inputEvent.type == INPUT_EVENT_KEYPAD) && (inputEvent.key == '#'))
                                {
                                    state = APP_STATE_PRODUCTION;
                                }

                                else if ((selectedProduct != PRODUCT_FILTER) && (userWallet >= requiredPrice))
                                {
                                    snprintf(line2, sizeof(line2), "%u DKK", userWallet-requiredPrice);
                                    //App_SendDisplay("Cashback amount", line2);
                                    state = APP_STATE_PAYBACK;
                                }
                            }


                            else if (inputEvent.type == (INPUT_EVENT_DIGI_RIGHT)) //|| (INPUT_EVENT_KEYPAD))
                            {
                                userWallet += 20;
                                snprintf(line2, sizeof(line2), "Paid: %u DKK", userWallet);
                                App_SendDisplay(App_ProductToText(selectedProduct), line2);
                                if ((selectedProduct == PRODUCT_FILTER) && (inputEvent.type == INPUT_EVENT_KEYPAD) && (inputEvent.key == '#'))
                               {
                                   state = APP_STATE_PRODUCTION;
                               }
                                else if ((selectedProduct != PRODUCT_FILTER) && (userWallet >= requiredPrice))
                                {
                                    snprintf(line2, sizeof(line2), "%u DKK", userWallet-requiredPrice);
                                    //App_SendDisplay("Cashback amount", line2);
                                    state = APP_STATE_PAYBACK;
                                }
                            }

                        }
                        break;

                    case APP_STATE_CARD:
                        if ((inputEvent.type == INPUT_EVENT_KEYPAD) && (cardSequence == PAYMENT_CARD_NUMBER))
                        {
                            if (inputEvent.key == '*')
                            {
                                if (numberIndex > 0)
                                {
                                    numberIndex--;
                                    cardNumber[numberIndex] = '\0';
                                    App_SendDisplay("Enter card no.", cardNumber);
                                }
                            }
                            else if ((inputEvent.key >= '0') && (inputEvent.key <= '9'))
                            {
                                if (numberIndex < 16)
                                {
                                    cardNumber[numberIndex] = inputEvent.key;
                                    numberIndex++;
                                    cardNumber[numberIndex] = '\0';

                                    App_SendDisplay("Enter card no.", cardNumber);

                                    if (numberIndex == 16)
                                    {
                                        cardSequence = PAYMENT_CARD_PIN;
                                        pinIndex = 0;
                                        pinCode[0] = '\0';
                                        App_SendDisplay("Enter pin. no", pinCode);
                                    }
                                }
                            }
                        }
                        else if ((inputEvent.type == INPUT_EVENT_KEYPAD) && (cardSequence == PAYMENT_CARD_PIN))
                        {
                            if (inputEvent.key == '*')
                            {
                                if (pinIndex > 0)
                                {
                                    pinIndex--;
                                    pinCode[pinIndex] = '\0';
                                    App_SendDisplay("Enter pin no.", pinCode);
                                }
                            }
                            else if ((inputEvent.key >= '0') && (inputEvent.key <= '9'))
                            {
                                if (pinIndex < 4)
                                {
                                    pinCode[pinIndex] = inputEvent.key;
                                    pinIndex++;
                                    pinCode[pinIndex] = '\0';

                                    App_SendDisplay("Enter pin no.", pinCode);

                                    if (pinIndex == 4)
                                    {
                                        if (App_IsDigitOdd(pinCode[pinIndex-1]) == App_IsDigitOdd(cardNumber[numberIndex-1]))
                                        {
                                            App_SendDisplay("Rune var", "tyk");
                                            state = APP_STATE_PRODUCTION;
                                        }
                                        else if (App_IsDigitOdd(pinCode[pinIndex-1]) != App_IsDigitOdd(cardNumber[numberIndex-1]))
                                        {
                                            App_SendDisplay("Credit card", "Rejected!");

                                            vTaskDelay(pdMS_TO_TICKS(1000));

                                            state = APP_STATE_IDLE;
                                        }
                                    }
                                }
                            }
                        }

                        break;

                    case APP_STATE_PAYBACK:
                    {
                        uint16_t cashbackAmount;
                        uint32_t blinkDurationMs;

                        cashbackAmount = userWallet - requiredPrice;
                        blinkDurationMs = (uint32_t)cashbackAmount * 1000U;

                        App_SendLed(LED_COLOR_GREEN, LED_MODE_BLINK, blinkDurationMs);
                        snprintf(line2, sizeof(line2), "%u DKK", cashbackAmount);
                        App_SendDisplay("Cashback amount", line2);

                        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                        state = APP_STATE_PRODUCTION;
                    }
                    break;

                    case APP_STATE_PRODUCTION:

                        if (cupPlaced == 0)
                        {
                            App_SendDisplay("Place cup", "Press SW1");
                        }

                        if (inputEvent.type == INPUT_EVENT_SW1)
                        {
                            cupPlaced = 1;
                            App_SendDisplay("Cup placed", "Press SW2");
                        }
                        else if (inputEvent.type == INPUT_EVENT_SW2)
                        {
                            if (cupPlaced == 0)
                            {
                                App_SendDisplay("No cup", "Place cup first");
                            }
                            else
                            {
                                if (selectedProduct == PRODUCT_ESPRESSO)
                                {
                                    App_SendDisplay("Grinding", "Espresso");
                                    App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON_FOR_TIME, 7500);

                                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
                                    //vTaskDelay(pdMS_TO_TICKS(7500));

                                    App_SendDisplay("Brewing", "Espresso");
                                    App_SendLed(LED_COLOR_RED, LED_MODE_ON_FOR_TIME, 14000);

                                    vTaskDelay(pdMS_TO_TICKS(14000));
                                }
                                else if (selectedProduct == PRODUCT_LATTE)
                                {
                                    App_SendDisplay("Grinding", "Latte");
                                    App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON_FOR_TIME, 7500);

                                    vTaskDelay(pdMS_TO_TICKS(7500));

                                    App_SendDisplay("Brewing", "Latte");
                                    App_SendLed(LED_COLOR_RED, LED_MODE_ON_FOR_TIME, 14000);

                                    vTaskDelay(pdMS_TO_TICKS(14000));

                                    App_SendDisplay("Frothing", "Milk");
                                    App_SendLed(LED_COLOR_GREEN, LED_MODE_ON_FOR_TIME, 6200);

                                    vTaskDelay(pdMS_TO_TICKS(6200));
                                }
                                else if (selectedProduct == PRODUCT_FILTER)
                                {
                                    // char line1[16];
                                    // char line1[16];
                                    App_SendDisplay("Filter coffee", "press to brew");
                                    if (inputEvent.type == INPUT_EVENT_SW1)
                                    {
                                        // clamout += 0.6
                                        // total += 1.8
                                        // sprintf(line1, "total", total, "price", 3);
                                        // sprintf(line2, "amount", clamount);
                                        // App_SendDisplay(line1, "");
                                        vTaskDelay(pdMS_TO_TICKS(1000));
                                    }
                                    App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON_FOR_TIME, 5000);

                                    vTaskDelay(pdMS_TO_TICKS(5000));
                                }

                                App_SendDisplay("Coffee ready", "Remove cup");

                                cupPlaced = 0;
                                selectedProduct = PRODUCT_NONE;
                                selectedPayment = PAYMENT_NONE;
                                userWallet = 0;

                                state = APP_STATE_IDLE;
                            }
                        }

                        break;

                    default:
                        state = APP_STATE_IDLE;
                        selectedProduct = PRODUCT_NONE;
                        selectedPayment = PAYMENT_NONE;
                        App_SendDisplay("Coffee Machine", "Press #");
                        break;
                }
            }
            while (state != previousState);
        }
        //
    }
}

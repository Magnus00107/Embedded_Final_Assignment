/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: app.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Main application state machine for coffee machine demo
*****************************************************************************/

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "app.h"
#include "app_types.h"
#include <stdio.h>
#include "loggerMain.h"

QueueHandle_t xInputQueue   = NULL;
QueueHandle_t xLcdQueue     = NULL;
QueueHandle_t xLedQueue     = NULL;
TaskHandle_t xAppTaskHandle = NULL;

uint8_t cupPlaced = 0;

static void App_SendDisplay(const char *line1, const char *line2)
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
        return prices->espresso_cup_dkk;

    case PRODUCT_LATTE:
        return prices->latte_cup_dkk;

    case PRODUCT_FILTER:
        return (uint16_t)(filterAmountCl * prices->filter_dkk_per_cl);

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
{
    return ((digit - '0') % 2) ? 1 : 0;
}

void App_Task(void *pvParameters)
{
    uint16_t cashbackAmount = 0;
    uint16_t filterAmountCl = 0;
    uint16_t filterTotalPrice = 0;
    uint16_t requiredPrice = 0;

    AppState_t state = APP_STATE_IDLE;
    AppState_t previousState;

    char cardNumber[17] = "";
    char pinCode[5] = "";

    uint8_t numberIndex = 0;
    uint8_t pinIndex = 0;
    uint8_t cardWalletEntry = 0;

    PaymentCard_t cardSequence = PAYMENT_CARD_NONE;
    Product_t selectedProduct = PRODUCT_NONE;
    Payment_t selectedPayment = PAYMENT_NONE;

    ProductPrices_t prices = {15, 27, 3};
    InputEvent_t inputEvent;

    uint16_t userWallet = 0;

    char line1[17];
    char line2[17];

    uint8_t buttonHold = 0;
    uint8_t filterTimerStarted = 0;
    uint8_t slowPhaseDone = 0;
    TickType_t filterStartTime = 0;
    uint16_t filterTickCounter = 0;
    TickType_t filterLastActivityTime = 0;
    uint8_t filterFinished = 0;

    (void)pvParameters;

    App_SendDisplay("Coffee Machine", "Press #");

    for (;;)
    {
        inputEvent.type = INPUT_EVENT_NONE;
        inputEvent.key = 0;

        if ((state == APP_STATE_PRODUCTION) && (selectedProduct == PRODUCT_FILTER))
        {
            xQueueReceive(xInputQueue, &inputEvent, pdMS_TO_TICKS(1000));
        }
        else
        {
            xQueueReceive(xInputQueue, &inputEvent, portMAX_DELAY);
        }

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
                            inputEvent.type = INPUT_EVENT_NONE;
                            inputEvent.key = 0;
                        }
                        else if (inputEvent.key == '2')
                        {
                            selectedProduct = PRODUCT_LATTE;
                            state = APP_STATE_SELECT_PAYMENT;
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
                        }
                        else if (inputEvent.key == '2')
                        {
                            selectedPayment = PAYMENT_CARD;
                            cardSequence = PAYMENT_CARD_NUMBER;
                            numberIndex = 0;
                            pinIndex = 0;
                            cardNumber[0] = '\0';
                            pinCode[0] = '\0';
                            state = APP_STATE_CARD;

                            App_SendDisplay("Enter card no.", cardNumber);

                            inputEvent.type = INPUT_EVENT_NONE;
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

                        if (inputEvent.type == INPUT_EVENT_DIGI_LEFT)
                        {
                            userWallet += 5;

                            snprintf(line2, sizeof(line2), "Paid: %u DKK", userWallet);
                            App_SendDisplay(App_ProductToText(selectedProduct), line2);

                            if ((selectedProduct != PRODUCT_FILTER) && (userWallet >= requiredPrice))
                            {
                                state = APP_STATE_PAYBACK;
                            }
                        }
                        else if (inputEvent.type == INPUT_EVENT_DIGI_RIGHT)
                        {
                            userWallet += 20;

                            snprintf(line2, sizeof(line2), "Paid: %u DKK", userWallet);
                            App_SendDisplay(App_ProductToText(selectedProduct), line2);

                            if ((selectedProduct != PRODUCT_FILTER) && (userWallet >= requiredPrice))
                            {
                                state = APP_STATE_PAYBACK;
                            }
                        }
                        else if ((selectedProduct == PRODUCT_FILTER) &&
                                 (inputEvent.type == INPUT_EVENT_KEYPAD) &&
                                 (inputEvent.key == '#') &&
                                 (userWallet > 0))
                        {
                            state = APP_STATE_PRODUCTION;
                        }
                    }
                    break;

                case APP_STATE_CARD:

                    if (cardWalletEntry == 1U)
                    {
                        if (inputEvent.type == INPUT_EVENT_KEYPAD)
                        {
                            if ((inputEvent.key >= '0') && (inputEvent.key <= '9'))
                            {
                                userWallet = (userWallet * 10U) + (uint16_t)(inputEvent.key - '0');

                                snprintf(line2, sizeof(line2), "%u DKK", userWallet);
                                App_SendDisplay("amount", line2);
                            }
                            else if ((inputEvent.key == '#') && (userWallet > 0U))
                            {
                                cardWalletEntry = 0U;
                                state = APP_STATE_PRODUCTION;
                            }
                            else if (inputEvent.key == '*')
                            {
                                userWallet = 0U;
                                App_SendDisplay("Enter amount", "Then press #");
                            }
                        }
                        break;
                    }

                    if ((inputEvent.type == INPUT_EVENT_KEYPAD) &&
                        (cardSequence == PAYMENT_CARD_NUMBER))
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
                                    App_SendDisplay("Enter pin no.", pinCode);
                                }
                            }
                        }
                    }
                    else if ((inputEvent.type == INPUT_EVENT_KEYPAD) &&
                             (cardSequence == PAYMENT_CARD_PIN))
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
                                    if (App_IsDigitOdd(pinCode[pinIndex - 1]) ==
                                        App_IsDigitOdd(cardNumber[numberIndex - 1]))
                                    {
                                        if (selectedProduct == PRODUCT_FILTER)
                                        {
                                            userWallet = 0U;
                                            cardWalletEntry = 1U;
                                            App_SendDisplay("Enter amount", "Then press #");
                                        }
                                        else
                                        {
                                            state = APP_STATE_PRODUCTION;
                                        }
                                    }
                                    else
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
                    uint32_t blinkDurationMs;

                    cashbackAmount = userWallet - requiredPrice;
                    blinkDurationMs = (uint32_t)cashbackAmount * 1000U;

                    App_SendLed(LED_COLOR_GREEN, LED_MODE_BLINK, blinkDurationMs);

                    snprintf(line2, sizeof(line2), "%u DKK", cashbackAmount);
                    App_SendDisplay("Cashback amount", line2);

                    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                    if (selectedProduct == PRODUCT_FILTER)
                    {
                        state = APP_STATE_DONE;
                    }
                    else
                    {
                        state = APP_STATE_PRODUCTION;
                    }
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

                    if ((selectedProduct == PRODUCT_FILTER) && (cupPlaced == 1))
                    {
                        if (inputEvent.type == INPUT_EVENT_SW2_PRESSED)
                        {
                            buttonHold = 1U;

                            filterStartTime = xTaskGetTickCount();
                            filterLastActivityTime = filterStartTime;
                            filterTimerStarted = 1U;
                            slowPhaseDone = 0U;
                            filterTickCounter = 0U;

                            App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON, 0U);
                        }
                        else if (inputEvent.type == INPUT_EVENT_SW2_RELEASED)
                        {
                            buttonHold = 0U;
                            filterLastActivityTime = xTaskGetTickCount();

                            App_SendLed(LED_COLOR_NONE, LED_MODE_OFF, 0U);
                        }

                        if ((filterTimerStarted == 1U) && (filterFinished == 0U))
                        {
                            if (buttonHold == 1U)
                            {
                                filterTickCounter++;

                                if ((slowPhaseDone == 0U) &&
                                    ((xTaskGetTickCount() - filterStartTime) >= pdMS_TO_TICKS(3000)))
                                {
                                    slowPhaseDone = 1U;
                                }

                                if (slowPhaseDone == 0U)
                                {
                                    filterAmountCl = filterAmountCl + 1U;
                                }
                                else
                                {
                                    filterAmountCl = filterAmountCl + 2U;
                                }

                                filterTotalPrice = filterAmountCl * prices.filter_dkk_per_cl;

                                if (filterTotalPrice >= userWallet)
                                {
                                    filterTotalPrice = userWallet;
                                    filterFinished = 1U;
                                }

                                snprintf(line1,
                                         sizeof(line1),
                                         "%u cl %u DKK/cl",
                                         filterAmountCl,
                                         prices.filter_dkk_per_cl);

                                snprintf(line2,
                                         sizeof(line2),
                                         "Total: %u DKK",
                                         filterTotalPrice);

                                App_SendDisplay(line1, line2);
                            }
                            else
                            {
                                if ((xTaskGetTickCount() - filterLastActivityTime) >= pdMS_TO_TICKS(5000))
                                {
                                    filterFinished = 1U;
                                }
                            }

                            if (filterFinished == 1U)
                            {
                                App_SendLed(LED_COLOR_NONE, LED_MODE_OFF, 0U);

                                requiredPrice = filterTotalPrice;

                                buttonHold = 0U;
                                filterTimerStarted = 0U;
                                slowPhaseDone = 0U;
                                filterTickCounter = 0U;

                                if ((selectedPayment == PAYMENT_CASH) && (userWallet > requiredPrice))
                                {
                                    state = APP_STATE_PAYBACK;
                                }
                                else
                                {
                                    state = APP_STATE_DONE;
                                }
                            }
                        }
                    }
                    else if (inputEvent.type == INPUT_EVENT_SW2_PRESSED)
                    {
                        if (cupPlaced == 0)
                        {
                            App_SendDisplay("No cup", "Place cup");
                            vTaskDelay(pdMS_TO_TICKS(2000));
                        }
                        else
                        {
                            if (selectedProduct == PRODUCT_ESPRESSO)
                            {
                                App_SendDisplay("Grinding", "Espresso");
                                App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON_FOR_TIME, 7500);

                                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                                App_SendDisplay("Brewing", "Espresso");
                                App_SendLed(LED_COLOR_RED, LED_MODE_ON_FOR_TIME, 14000);

                                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                                state = APP_STATE_DONE;
                            }
                            else if (selectedProduct == PRODUCT_LATTE)
                            {
                                App_SendDisplay("Grinding", "Latte");
                                App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON_FOR_TIME, 7500);

                                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                                App_SendDisplay("Brewing", "Latte");
                                App_SendLed(LED_COLOR_RED, LED_MODE_ON_FOR_TIME, 14000);

                                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                                App_SendDisplay("Frothing", "Milk");
                                App_SendLed(LED_COLOR_GREEN, LED_MODE_ON_FOR_TIME, 6200);

                                ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

                                state = APP_STATE_DONE;
                            }
                        }
                    }
                    break;

                case APP_STATE_DONE:

                    App_SendDisplay("Coffee ready", "Remove cup");

                    if (inputEvent.type == INPUT_EVENT_SW1)
                    {
                        LogEntry logEntry;

                        logEntry.product = selectedProduct;
                        logEntry.payment = selectedPayment;

                        if (selectedProduct == PRODUCT_FILTER)
                        {
                            logEntry.amount = filterAmountCl;
                        }
                        else
                        {
                            logEntry.amount = 1;
                        }

                        logEntry.price = requiredPrice;

                        if (selectedPayment == PAYMENT_CASH)
                        {
                            strcpy(logEntry.paymentInfo, "CASH");
                        }
                        else if (selectedPayment == PAYMENT_CARD)
                        {
                            strncpy(logEntry.paymentInfo, cardNumber, sizeof(logEntry.paymentInfo) - 1);
                            logEntry.paymentInfo[sizeof(logEntry.paymentInfo) - 1] = '\0';
                        }
                        else
                        {
                            strcpy(logEntry.paymentInfo, "NONE");
                        }

                        taskENTER_CRITICAL();
                        logEntry.hours   = timeOfDay.hours;
                        logEntry.minutes = timeOfDay.minutes;
                        logEntry.seconds = timeOfDay.seconds;
                        taskEXIT_CRITICAL();

                        xQueueSend(logQueue, &logEntry, pdMS_TO_TICKS(100));
                        cupPlaced = 0;

                        selectedProduct = PRODUCT_NONE;
                        selectedPayment = PAYMENT_NONE;
                        cardSequence = PAYMENT_CARD_NONE;

                        userWallet = 0U;
                        requiredPrice = 0U;
                        cashbackAmount = 0U;

                        filterAmountCl = 0U;
                        filterTotalPrice = 0U;
                        filterFinished = 0U;
                        filterTimerStarted = 0U;
                        buttonHold = 0U;
                        slowPhaseDone = 0U;
                        filterTickCounter = 0U;
                        cardWalletEntry = 0U;

                        numberIndex = 0U;
                        pinIndex = 0U;
                        cardNumber[0] = '\0';
                        pinCode[0] = '\0';

                        state = APP_STATE_IDLE;
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
}

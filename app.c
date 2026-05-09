/**********************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME: app.c
* PROJECT: Poster Assignment
* DESCRIPTION: Main application state machine for coffee machine demo.
*
* Change log:
***********************************************
* Date of Change
* YYMMDD
* ----------------
**********************************************/

/***************** Include files **************/
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "app.h"
#include "app_types.h"
#include "loggerMain.h"

/***************** Defines ********************/
#define LCD_TEXT_LENGTH                 16U
#define LCD_BUFFER_LENGTH               (LCD_TEXT_LENGTH + 1U)

#define CARD_NUMBER_LENGTH              16U
#define PIN_CODE_LENGTH                 4U
#define CARD_BUFFER_LENGTH              (CARD_NUMBER_LENGTH + 1U)
#define PIN_BUFFER_LENGTH               (PIN_CODE_LENGTH + 1U)

#define COIN_SMALL_DKK                  5U
#define COIN_LARGE_DKK                  20U

#define ESPRESSO_PRICE_DKK              15U
#define LATTE_PRICE_DKK                 27U
#define FILTER_PRICE_DKK_PER_CL         3U

#define FILTER_QUEUE_TIMEOUT_MS         1000U
#define FILTER_SLOW_PHASE_MS            3000U
#define FILTER_IDLE_FINISH_MS           5000U

#define CARD_REJECT_DISPLAY_MS          1000U
#define NO_CUP_DISPLAY_MS               2000U

#define PAYBACK_BLINK_MS_PER_DKK        1000U
#define GRINDING_DURATION_MS            7500U
#define BREWING_DURATION_MS             14000U
#define FROTHING_DURATION_MS            6200U

#define LOG_QUEUE_TIMEOUT_MS            100U

#define TRUE_U8                         1U
#define FALSE_U8                        0U

/***************** Constants ******************/

/***************** Variables ******************/
QueueHandle_t xInputQueue   = NULL;
QueueHandle_t xLcdQueue     = NULL;
QueueHandle_t xLedQueue     = NULL;
TaskHandle_t xAppTaskHandle = NULL;

uint8_t cupPlaced = FALSE_U8;

typedef struct
{
    uint16_t cashbackAmount;
    uint16_t filterAmountCl;
    uint16_t filterTotalPrice;
    uint16_t requiredPrice;
    uint16_t userWallet;

    AppState_t state;
    AppState_t previousState;

    char cardNumber[CARD_BUFFER_LENGTH];
    char pinCode[PIN_BUFFER_LENGTH];
    char line1[LCD_BUFFER_LENGTH];
    char line2[LCD_BUFFER_LENGTH];

    uint8_t numberIndex;
    uint8_t pinIndex;
    uint8_t cardWalletEntry;

    PaymentCard_t cardSequence;
    Product_t selectedProduct;
    Payment_t selectedPayment;

    ProductPrices_t prices;
    InputEvent_t inputEvent;

    uint8_t buttonHold;
    uint8_t filterTimerStarted;
    uint8_t slowPhaseDone;
    TickType_t filterStartTime;
    uint16_t filterTickCounter;
    TickType_t filterLastActivityTime;
    uint8_t filterFinished;
} AppContext_t;

/***************** Functions ******************/
static void App_SendDisplay(const char *line1, const char *line2)
/**********************************************
* Input:    line1 - text for the first LCD line.
*           line2 - text for the second LCD line.
* Output:   None.
* Function: Sends a two-line display message to the LCD queue.
**********************************************/
{
    LcdMessage_t lcdMessage;

    memset(&lcdMessage, 0, sizeof(LcdMessage_t));

    strncpy(lcdMessage.line1, line1, LCD_TEXT_LENGTH);
    strncpy(lcdMessage.line2, line2, LCD_TEXT_LENGTH);

    lcdMessage.line1[LCD_TEXT_LENGTH] = '\0';
    lcdMessage.line2[LCD_TEXT_LENGTH] = '\0';

    xQueueSend(xLcdQueue, &lcdMessage, portMAX_DELAY);
}

static const char *App_ProductToText(Product_t product)
/**********************************************
* Input:    product - selected product value.
* Output:   Text matching the product value.
* Function: Converts a product value to display text.
**********************************************/
{
    const char *text = "None";

    switch (product)
    {
        case PRODUCT_ESPRESSO:
            text = "Espresso";
            break;

        case PRODUCT_LATTE:
            text = "Latte";
            break;

        case PRODUCT_FILTER:
            text = "Filter";
            break;

        case PRODUCT_NONE:
        default:
            text = "None";
            break;
    }

    return text;
}

static uint16_t App_GetProductPrice(Product_t product,
                                    ProductPrices_t *prices,
                                    uint16_t filterAmountCl)
/**********************************************
* Input:    product        - selected product value.
*           prices         - pointer to product price table.
*           filterAmountCl - selected filter coffee amount in cl.
* Output:   Product price in DKK.
* Function: Calculates the required price for the selected product.
**********************************************/
{
    uint16_t price = 0U;

    switch (product)
    {
        case PRODUCT_ESPRESSO:
            price = prices->espresso_cup_dkk;
            break;

        case PRODUCT_LATTE:
            price = prices->latte_cup_dkk;
            break;

        case PRODUCT_FILTER:
            price = (uint16_t)(filterAmountCl * prices->filter_dkk_per_cl);
            break;

        case PRODUCT_NONE:
        default:
            price = 0U;
            break;
    }

    return price;
}

static void App_SendLed(LedColor_t color, LedMode_t mode, uint32_t durationMs)
/**********************************************
* Input:    color      - LED color.
*           mode       - LED mode.
*           durationMs - LED duration in milliseconds.
* Output:   None.
* Function: Sends an LED command to the LED queue.
**********************************************/
{
    LedMessage_t ledMessage;

    ledMessage.color = color;
    ledMessage.mode = mode;
    ledMessage.duration_ms = durationMs;

    xQueueSend(xLedQueue, &ledMessage, portMAX_DELAY);
}

static uint8_t App_IsDigitOdd(char digit)
/**********************************************
* Input:    digit - ASCII digit character.
* Output:   TRUE_U8 if digit is odd, otherwise FALSE_U8.
* Function: Checks whether a digit character is an odd number.
**********************************************/
{
    uint8_t isOdd;

    isOdd = (uint8_t)(((digit - '0') % 2) ? TRUE_U8 : FALSE_U8);

    return isOdd;
}

static void App_RunProductionStep(const char *line1,
                                  const char *line2,
                                  LedColor_t color,
                                  uint32_t durationMs)
/**********************************************
* Input:    line1      - first LCD line.
*           line2      - second LCD line.
*           color      - LED color for the step.
*           durationMs - step duration in milliseconds.
* Output:   None.
* Function: Runs one timed production step and waits for completion.
**********************************************/
{
    App_SendDisplay(line1, line2);
    App_SendLed(color, LED_MODE_ON_FOR_TIME, durationMs);

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

static void App_ClearInputEvent(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Clears the currently processed input event.
**********************************************/
{
    context->inputEvent.type = INPUT_EVENT_NONE;
    context->inputEvent.key = 0;
}

static void App_InitContext(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Initializes all application state variables.
**********************************************/
{
    memset(context, 0, sizeof(AppContext_t));

    context->state = APP_STATE_IDLE;
    context->previousState = APP_STATE_IDLE;
    context->cardSequence = PAYMENT_CARD_NONE;
    context->selectedProduct = PRODUCT_NONE;
    context->selectedPayment = PAYMENT_NONE;
    context->prices.espresso_cup_dkk = ESPRESSO_PRICE_DKK;
    context->prices.latte_cup_dkk = LATTE_PRICE_DKK;
    context->prices.filter_dkk_per_cl = FILTER_PRICE_DKK_PER_CL;
}

static void App_ReceiveInput(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Receives input, using timeout during filter production.
**********************************************/
{
    App_ClearInputEvent(context);

    if ((context->state == APP_STATE_PRODUCTION) &&
        (context->selectedProduct == PRODUCT_FILTER))
    {
        xQueueReceive(xInputQueue,
                      &context->inputEvent,
                      pdMS_TO_TICKS(FILTER_QUEUE_TIMEOUT_MS));
    }
    else
    {
        xQueueReceive(xInputQueue, &context->inputEvent, portMAX_DELAY);
    }
}

static void App_HandleIdleState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles the idle state.
**********************************************/
{
    App_SendDisplay("Coffee Machine", "Press #");

    if ((context->inputEvent.type == INPUT_EVENT_KEYPAD) &&
        (context->inputEvent.key == '#'))
    {
        context->state = APP_STATE_SELECT_PRODUCT;
    }
}

static void App_HandleSelectProductState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles product selection.
**********************************************/
{
    App_SendDisplay("1:E 2:L 3:F", "Select product");

    if (context->inputEvent.type == INPUT_EVENT_KEYPAD)
    {
        if (context->inputEvent.key == '1')
        {
            context->selectedProduct = PRODUCT_ESPRESSO;
            context->state = APP_STATE_SELECT_PAYMENT;
            App_ClearInputEvent(context);
        }
        else if (context->inputEvent.key == '2')
        {
            context->selectedProduct = PRODUCT_LATTE;
            context->state = APP_STATE_SELECT_PAYMENT;
            App_ClearInputEvent(context);
        }
        else if (context->inputEvent.key == '3')
        {
            context->selectedProduct = PRODUCT_FILTER;
            context->state = APP_STATE_SELECT_PAYMENT;
            App_ClearInputEvent(context);
        }
    }
}

static void App_HandleSelectPaymentState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles payment type selection.
**********************************************/
{
    App_SendDisplay(App_ProductToText(context->selectedProduct), "1:Cash 2:Card");

    if (context->inputEvent.type == INPUT_EVENT_KEYPAD)
    {
        if (context->inputEvent.key == '1')
        {
            context->selectedPayment = PAYMENT_CASH;
            context->userWallet = 0U;
            context->state = APP_STATE_TRANSACTION;
        }
        else if (context->inputEvent.key == '2')
        {
            context->selectedPayment = PAYMENT_CARD;
            context->cardSequence = PAYMENT_CARD_NUMBER;
            context->numberIndex = 0U;
            context->pinIndex = 0U;
            context->cardNumber[0] = '\0';
            context->pinCode[0] = '\0';
            context->state = APP_STATE_CARD;

            App_SendDisplay("Enter card no.", context->cardNumber);
            App_ClearInputEvent(context);
        }
    }
}

static void App_HandleTransactionState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles the transaction state.
**********************************************/
{
    context->requiredPrice = App_GetProductPrice(context->selectedProduct,
                                                 &context->prices,
                                                 context->filterAmountCl);

    if (context->selectedPayment == PAYMENT_CASH)
    {
        if (context->userWallet == 0U)
        {
            App_SendDisplay(App_ProductToText(context->selectedProduct), "Insert coins");
        }

        if (context->inputEvent.type == INPUT_EVENT_DIGI_LEFT)
        {
            context->userWallet = (uint16_t)(context->userWallet + COIN_SMALL_DKK);

            snprintf(context->line2,
                     sizeof(context->line2),
                     "Paid: %u DKK",
                     context->userWallet);

            App_SendDisplay(App_ProductToText(context->selectedProduct), context->line2);

            if ((context->selectedProduct != PRODUCT_FILTER) &&
                (context->userWallet >= context->requiredPrice))
            {
                context->state = APP_STATE_PAYBACK;
            }
        }
        else if (context->inputEvent.type == INPUT_EVENT_DIGI_RIGHT)
        {
            context->userWallet = (uint16_t)(context->userWallet + COIN_LARGE_DKK);

            snprintf(context->line2,
                     sizeof(context->line2),
                     "Paid: %u DKK",
                     context->userWallet);

            App_SendDisplay(App_ProductToText(context->selectedProduct), context->line2);

            if ((context->selectedProduct != PRODUCT_FILTER) &&
                (context->userWallet >= context->requiredPrice))
            {
                context->state = APP_STATE_PAYBACK;
            }
        }
        else if ((context->selectedProduct == PRODUCT_FILTER) &&
                 (context->inputEvent.type == INPUT_EVENT_KEYPAD) &&
                 (context->inputEvent.key == '#') &&
                 (context->userWallet > 0U))
        {
            context->state = APP_STATE_PRODUCTION;
        }
    }
}

static void App_HandleCardState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles card number, PIN, and amount entry.
**********************************************/
{
    if (context->cardWalletEntry == TRUE_U8)
    {
        if (context->inputEvent.type == INPUT_EVENT_KEYPAD)
        {
            if ((context->inputEvent.key >= '0') && (context->inputEvent.key <= '9'))
            {
                context->userWallet = (uint16_t)((context->userWallet * 10U) +
                                                 (uint16_t)(context->inputEvent.key - '0'));

                snprintf(context->line2,
                         sizeof(context->line2),
                         "%u DKK",
                         context->userWallet);

                App_SendDisplay("amount", context->line2);
            }
            else if ((context->inputEvent.key == '#') && (context->userWallet > 0U))
            {
                context->cardWalletEntry = FALSE_U8;
                context->state = APP_STATE_PRODUCTION;
            }
            else if (context->inputEvent.key == '*')
            {
                context->userWallet = 0U;
                App_SendDisplay("Enter amount", "Then press #");
            }
        }
    }
    else if ((context->inputEvent.type == INPUT_EVENT_KEYPAD) &&
             (context->cardSequence == PAYMENT_CARD_NUMBER))
    {
        if (context->inputEvent.key == '*')
        {
            if (context->numberIndex > 0U)
            {
                context->numberIndex--;
                context->cardNumber[context->numberIndex] = '\0';
                App_SendDisplay("Enter card no.", context->cardNumber);
            }
        }
        else if ((context->inputEvent.key >= '0') && (context->inputEvent.key <= '9'))
        {
            if (context->numberIndex < CARD_NUMBER_LENGTH)
            {
                context->cardNumber[context->numberIndex] = context->inputEvent.key;
                context->numberIndex++;
                context->cardNumber[context->numberIndex] = '\0';

                App_SendDisplay("Enter card no.", context->cardNumber);

                if (context->numberIndex == CARD_NUMBER_LENGTH)
                {
                    context->cardSequence = PAYMENT_CARD_PIN;
                    context->pinIndex = 0U;
                    context->pinCode[0] = '\0';
                    App_SendDisplay("Enter pin no.", context->pinCode);
                }
            }
        }
    }
    else if ((context->inputEvent.type == INPUT_EVENT_KEYPAD) &&
             (context->cardSequence == PAYMENT_CARD_PIN))
    {
        if (context->inputEvent.key == '*')
        {
            if (context->pinIndex > 0U)
            {
                context->pinIndex--;
                context->pinCode[context->pinIndex] = '\0';
                App_SendDisplay("Enter pin no.", context->pinCode);
            }
        }
        else if ((context->inputEvent.key >= '0') && (context->inputEvent.key <= '9'))
        {
            if (context->pinIndex < PIN_CODE_LENGTH)
            {
                context->pinCode[context->pinIndex] = context->inputEvent.key;
                context->pinIndex++;
                context->pinCode[context->pinIndex] = '\0';

                App_SendDisplay("Enter pin no.", context->pinCode);

                if (context->pinIndex == PIN_CODE_LENGTH)
                {
                    if (App_IsDigitOdd(context->pinCode[context->pinIndex - 1U]) ==
                        App_IsDigitOdd(context->cardNumber[context->numberIndex - 1U]))
                    {
                        if (context->selectedProduct == PRODUCT_FILTER)
                        {
                            context->userWallet = 0U;
                            context->cardWalletEntry = TRUE_U8;
                            App_SendDisplay("Enter amount", "Then press #");
                        }
                        else
                        {
                            context->state = APP_STATE_PRODUCTION;
                        }
                    }
                    else
                    {
                        App_SendDisplay("Credit card", "Rejected!");
                        vTaskDelay(pdMS_TO_TICKS(CARD_REJECT_DISPLAY_MS));
                        context->state = APP_STATE_IDLE;
                    }
                }
            }
        }
    }
}

static void App_HandlePaybackState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles cashback calculation and display.
**********************************************/
{
    uint32_t blinkDurationMs;

    context->cashbackAmount = (uint16_t)(context->userWallet - context->requiredPrice);
    blinkDurationMs = (uint32_t)context->cashbackAmount * PAYBACK_BLINK_MS_PER_DKK;

    App_SendLed(LED_COLOR_GREEN, LED_MODE_BLINK, blinkDurationMs);

    snprintf(context->line2,
             sizeof(context->line2),
             "%u DKK",
             context->cashbackAmount);

    App_SendDisplay("Cashback amount", context->line2);

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (context->selectedProduct == PRODUCT_FILTER)
    {
        context->state = APP_STATE_DONE;
    }
    else
    {
        context->state = APP_STATE_PRODUCTION;
    }
}

static void App_HandleProductionState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles cup placement and product production.
**********************************************/
{
    if (cupPlaced == FALSE_U8)
    {
        App_SendDisplay("Place cup", "Press SW1");
    }

    if (context->inputEvent.type == INPUT_EVENT_SW1)
    {
        cupPlaced = TRUE_U8;
        App_SendDisplay("Cup placed", "Press SW2");
    }

    if ((context->selectedProduct == PRODUCT_FILTER) && (cupPlaced == TRUE_U8))
    {
        if (context->inputEvent.type == INPUT_EVENT_SW2_PRESSED)
        {
            context->buttonHold = TRUE_U8;
            context->filterStartTime = xTaskGetTickCount();
            context->filterLastActivityTime = context->filterStartTime;
            context->filterTimerStarted = TRUE_U8;
            context->slowPhaseDone = FALSE_U8;
            context->filterTickCounter = 0U;

            App_SendLed(LED_COLOR_YELLOW, LED_MODE_ON, 0U);
        }
        else if (context->inputEvent.type == INPUT_EVENT_SW2_RELEASED)
        {
            context->buttonHold = FALSE_U8;
            context->filterLastActivityTime = xTaskGetTickCount();

            App_SendLed(LED_COLOR_NONE, LED_MODE_OFF, 0U);
        }

        if ((context->filterTimerStarted == TRUE_U8) &&
            (context->filterFinished == FALSE_U8))
        {
            if (context->buttonHold == TRUE_U8)
            {
                context->filterTickCounter++;

                if ((context->slowPhaseDone == FALSE_U8) &&
                    ((xTaskGetTickCount() - context->filterStartTime) >=
                     pdMS_TO_TICKS(FILTER_SLOW_PHASE_MS)))
                {
                    context->slowPhaseDone = TRUE_U8;
                }

                if (context->slowPhaseDone == FALSE_U8)
                {
                    context->filterAmountCl = (uint16_t)(context->filterAmountCl + 1U);
                }
                else
                {
                    context->filterAmountCl = (uint16_t)(context->filterAmountCl + 2U);
                }

                context->filterTotalPrice = (uint16_t)(context->filterAmountCl *
                                                       context->prices.filter_dkk_per_cl);

                if (context->filterTotalPrice >= context->userWallet)
                {
                    context->filterTotalPrice = context->userWallet;
                    context->filterFinished = TRUE_U8;
                }

                snprintf(context->line1,
                         sizeof(context->line1),
                         "%u cl %u DKK/cl",
                         context->filterAmountCl,
                         context->prices.filter_dkk_per_cl);

                snprintf(context->line2,
                         sizeof(context->line2),
                         "Total: %u DKK",
                         context->filterTotalPrice);

                App_SendDisplay(context->line1, context->line2);
            }
            else
            {
                if ((xTaskGetTickCount() - context->filterLastActivityTime) >=
                    pdMS_TO_TICKS(FILTER_IDLE_FINISH_MS))
                {
                    context->filterFinished = TRUE_U8;
                }
            }

            if (context->filterFinished == TRUE_U8)
            {
                App_SendLed(LED_COLOR_NONE, LED_MODE_OFF, 0U);

                context->requiredPrice = context->filterTotalPrice;
                context->buttonHold = FALSE_U8;
                context->filterTimerStarted = FALSE_U8;
                context->slowPhaseDone = FALSE_U8;
                context->filterTickCounter = 0U;

                if ((context->selectedPayment == PAYMENT_CASH) &&
                    (context->userWallet > context->requiredPrice))
                {
                    context->state = APP_STATE_PAYBACK;
                }
                else
                {
                    context->state = APP_STATE_DONE;
                }
            }
        }
    }
    else if (context->inputEvent.type == INPUT_EVENT_SW2_PRESSED)
    {
        if (cupPlaced == FALSE_U8)
        {
            App_SendDisplay("No cup", "Place cup");
            vTaskDelay(pdMS_TO_TICKS(NO_CUP_DISPLAY_MS));
        }
        else
        {
            if (context->selectedProduct == PRODUCT_ESPRESSO)
            {
                App_RunProductionStep("Grinding",
                                      "Espresso",
                                      LED_COLOR_YELLOW,
                                      GRINDING_DURATION_MS);

                App_RunProductionStep("Brewing",
                                      "Espresso",
                                      LED_COLOR_RED,
                                      BREWING_DURATION_MS);

                context->state = APP_STATE_DONE;
            }
            else if (context->selectedProduct == PRODUCT_LATTE)
            {
                App_RunProductionStep("Grinding",
                                      "Latte",
                                      LED_COLOR_YELLOW,
                                      GRINDING_DURATION_MS);

                App_RunProductionStep("Brewing",
                                      "Latte",
                                      LED_COLOR_RED,
                                      BREWING_DURATION_MS);

                App_RunProductionStep("Frothing",
                                      "Milk",
                                      LED_COLOR_GREEN,
                                      FROTHING_DURATION_MS);

                context->state = APP_STATE_DONE;
            }
        }
    }
}

static void App_HandleDoneState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Handles completion, logging, and reset after cup removal.
**********************************************/
{
    LogEntry logEntry;

    App_SendDisplay("Coffee ready", "Remove cup");

    if (context->inputEvent.type == INPUT_EVENT_SW1)
    {
        logEntry.product = context->selectedProduct;
        logEntry.payment = context->selectedPayment;

        if (context->selectedProduct == PRODUCT_FILTER)
        {
            logEntry.amount = context->filterAmountCl;
        }
        else
        {
            logEntry.amount = 1U;
        }

        logEntry.price = context->requiredPrice;

        if (context->selectedPayment == PAYMENT_CASH)
        {
            strcpy(logEntry.paymentInfo, "CASH");
        }
        else if (context->selectedPayment == PAYMENT_CARD)
        {
            strncpy(logEntry.paymentInfo,
                    context->cardNumber,
                    sizeof(logEntry.paymentInfo) - 1U);
            logEntry.paymentInfo[sizeof(logEntry.paymentInfo) - 1U] = '\0';
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

        xQueueSend(logQueue, &logEntry, pdMS_TO_TICKS(LOG_QUEUE_TIMEOUT_MS));

        cupPlaced = FALSE_U8;

        context->selectedProduct = PRODUCT_NONE;
        context->selectedPayment = PAYMENT_NONE;
        context->cardSequence = PAYMENT_CARD_NONE;

        context->userWallet = 0U;
        context->requiredPrice = 0U;
        context->cashbackAmount = 0U;

        context->filterAmountCl = 0U;
        context->filterTotalPrice = 0U;
        context->filterFinished = FALSE_U8;
        context->filterTimerStarted = FALSE_U8;
        context->buttonHold = FALSE_U8;
        context->slowPhaseDone = FALSE_U8;
        context->filterTickCounter = 0U;
        context->cardWalletEntry = FALSE_U8;

        context->numberIndex = 0U;
        context->pinIndex = 0U;
        context->cardNumber[0] = '\0';
        context->pinCode[0] = '\0';

        context->state = APP_STATE_IDLE;
    }
}

static void App_HandleDefaultState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Resets invalid state to idle.
**********************************************/
{
    context->state = APP_STATE_IDLE;
    context->selectedProduct = PRODUCT_NONE;
    context->selectedPayment = PAYMENT_NONE;
    App_SendDisplay("Coffee Machine", "Press #");
}

static void App_HandleCurrentState(AppContext_t *context)
/**********************************************
* Input:    context - pointer to application context.
* Output:   None.
* Function: Dispatches handling for the current state.
**********************************************/
{
    switch (context->state)
    {
        case APP_STATE_IDLE:
            App_HandleIdleState(context);
            break;

        case APP_STATE_SELECT_PRODUCT:
            App_HandleSelectProductState(context);
            break;

        case APP_STATE_SELECT_PAYMENT:
            App_HandleSelectPaymentState(context);
            break;

        case APP_STATE_TRANSACTION:
            App_HandleTransactionState(context);
            break;

        case APP_STATE_CARD:
            App_HandleCardState(context);
            break;

        case APP_STATE_PAYBACK:
            App_HandlePaybackState(context);
            break;

        case APP_STATE_PRODUCTION:
            App_HandleProductionState(context);
            break;

        case APP_STATE_DONE:
            App_HandleDoneState(context);
            break;

        default:
            App_HandleDefaultState(context);
            break;
    }
}

void App_Task(void *pvParameters)
/**********************************************
* Input:    pvParameters - FreeRTOS task parameter, not used.
* Output:   None.
* Function: Runs the main coffee machine application state machine.
**********************************************/
{
    AppContext_t context;

    (void)pvParameters;

    App_InitContext(&context);
    App_SendDisplay("Coffee Machine", "Press #");

    for (;;)
    {
        App_ReceiveInput(&context);

        do
        {
            context.previousState = context.state;
            App_HandleCurrentState(&context);
        }
        while (context.state != context.previousState);
    }
}

/***************** End of module **************/

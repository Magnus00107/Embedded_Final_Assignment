/*****************************************************************************
* University of Southern Denmark
* Embedded C Programming (ECP)
*
* MODULENAME.: loggerMain.c
*
* PROJECT....: Poster Assignment
*
* DESCRIPTION: Logger, UART command handling, sales report and clock tasks.
*
* Change Log:
******************************************************************************
* Date    Id    Change
* YYMMDD
* --------------------
* 260430  MoH   Module created.
*
*****************************************************************************/

/***************************** Include files *******************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "loggerMain.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "tm4c123gh6pm.h"
#include "app_types.h"

/*****************************    Defines    *******************************/

#define UART_MSG_SIZE               120U
#define CMD_BUFFER_SIZE             50U
#define CMD_LAST_INDEX              (CMD_BUFFER_SIZE - 1U)

#define UART_QUEUE_WAIT_MS          100U
#define COMMAND_TASK_DELAY_MS       10U
#define CLOCK_TASK_DELAY_MS         1000U

#define UART0_RX_EMPTY              0x10U
#define UART0_TX_FULL               0x20U

#define UART0_CLOCK_ENABLE          0x01U
#define GPIO_PORT_A_ENABLE          0x01U
#define GPIO_PA0_PA1_MASK           0x03U
#define GPIO_PCTL_PA0_PA1_MASK      0xFFFFFF00U
#define GPIO_PCTL_UART0_PA0_PA1     0x00000011U

#define UART0_DISABLE               0x01U
#define UART0_ENABLE                0x301U
#define UART0_BAUD_INT              8U
#define UART0_BAUD_FRAC             44U
#define UART0_LINE_CONTROL          0x70U
#define UART0_DATA_MASK             0xFFU

#define TIME_SECONDS_MAX            60U
#define TIME_MINUTES_MAX            60U
#define TIME_HOURS_MAX              24U

#define DEFAULT_ESPRESSO_PRICE      15U
#define DEFAULT_LATTE_PRICE         27U
#define DEFAULT_FILTER_PRICE        3U

/*****************************   Constants   *******************************/

/*****************************   Variables   *******************************/

QueueHandle_t uartQueue = NULL;
QueueHandle_t logQueue = NULL;

ProductPrices_t prices =
{
    DEFAULT_ESPRESSO_PRICE,
    DEFAULT_LATTE_PRICE,
    DEFAULT_FILTER_PRICE
};

SalesReport_t salesReport = {0};
SystemTime systemTime = {0, 0, 0};
TimeOfDay_t timeOfDay = {0, 0, 0};

/*****************************   Functions   *******************************/

static void logger_send_uart_message(char *message)
/*****************************************************************************
*   Input    : Pointer to message string
*   Output   : -
*   Function : Sends a text message to the UART queue
******************************************************************************/
{
    xQueueSend(uartQueue, &message, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));
}

/****************************** End Of Function ****************************/

static void logger_copy_time_to_entry(LogEntry *entry)
/*****************************************************************************
*   Input    : Pointer to log entry
*   Output   : -
*   Function : Copies current time of day into a log entry
******************************************************************************/
{
    taskENTER_CRITICAL();
    entry->hours = timeOfDay.hours;
    entry->minutes = timeOfDay.minutes;
    entry->seconds = timeOfDay.seconds;
    taskEXIT_CRITICAL();
}

/****************************** End Of Function ****************************/

static void logger_create_sale(Product_t product,
                               uint8_t amount,
                               uint16_t price,
                               Payment_t payment,
                               const char *payment_info)
/*****************************************************************************
*   Input    : Product, amount, price, payment type and payment information
*   Output   : -
*   Function : Creates and sends a log entry to the log queue
******************************************************************************/
{
    LogEntry entry;

    entry.product = product;
    entry.amount = amount;
    entry.price = price;
    entry.payment = payment;

    strncpy(entry.paymentInfo, payment_info, sizeof(entry.paymentInfo) - 1U);
    entry.paymentInfo[sizeof(entry.paymentInfo) - 1U] = '\0';

    logger_copy_time_to_entry(&entry);

    xQueueSend(logQueue, &entry, portMAX_DELAY);
}

/****************************** End Of Function ****************************/

static void logger_increment_time(TimeOfDay_t *time_value)
/*****************************************************************************
*   Input    : Pointer to time value
*   Output   : -
*   Function : Increments a HH:MM:SS time value by one second
******************************************************************************/
{
    time_value->seconds++;

    if (time_value->seconds >= TIME_SECONDS_MAX)
    {
        time_value->seconds = 0U;
        time_value->minutes++;
    }

    if (time_value->minutes >= TIME_MINUTES_MAX)
    {
        time_value->minutes = 0U;
        time_value->hours++;
    }

    if (time_value->hours >= TIME_HOURS_MAX)
    {
        time_value->hours = 0U;
    }
}

/****************************** End Of Function ****************************/

const char *ProductToString(Product_t product)
/*****************************************************************************
*   Input    : Product type
*   Output   : Pointer to product text
*   Function : Converts a product enum to readable text
******************************************************************************/
{
    const char *product_text = "UNKNOWN";

    switch (product)
    {
        case PRODUCT_ESPRESSO:
            product_text = "ESPRESSO";
            break;

        case PRODUCT_LATTE:
            product_text = "LATTE";
            break;

        case PRODUCT_FILTER:
            product_text = "FILTER";
            break;

        default:
            break;
    }

    return product_text;
}

/****************************** End Of Function ****************************/

const char *PaymentToString(Payment_t payment)
/*****************************************************************************
*   Input    : Payment type
*   Output   : Pointer to payment text
*   Function : Converts a payment enum to readable text
******************************************************************************/
{
    const char *payment_text = "NONE";

    switch (payment)
    {
        case PAYMENT_CASH:
            payment_text = "CASH";
            break;

        case PAYMENT_CARD:
            payment_text = "CARD";
            break;

        default:
            break;
    }

    return payment_text;
}

/****************************** End Of Function ****************************/

int UART0_CharAvailable(void)
/*****************************************************************************
*   Input    : -
*   Output   : Non-zero if a UART character is available
*   Function : Checks if UART0 has received data
******************************************************************************/
{
    int char_available = 0;

    if ((UART0_FR_R & UART0_RX_EMPTY) == 0U)
    {
        char_available = 1;
    }

    return char_available;
}

/****************************** End Of Function ****************************/

char UART0_GetChar(void)
/*****************************************************************************
*   Input    : -
*   Output   : Received UART character
*   Function : Waits for and reads one character from UART0
******************************************************************************/
{
    char received_char;

    while ((UART0_FR_R & UART0_RX_EMPTY) != 0U)
    {
    }

    received_char = (char)(UART0_DR_R & UART0_DATA_MASK);

    return received_char;
}

/****************************** End Of Function ****************************/

void ParseCommand(char *cmd)
/*****************************************************************************
*   Input    : Command string
*   Output   : -
*   Function : Parses UART commands and updates logger state
******************************************************************************/
{
    char msg[UART_MSG_SIZE];
    int value;
    int hours;
    int minutes;
    int seconds;
    uint8_t command_handled = 0U;

    if (strcmp(cmd, "REPORT") == 0)
    {
        snprintf(msg, UART_MSG_SIZE, "REPORT:\r\n");
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "ESPRESSO SOLD=%u REV=%lu DKK\r\n",
                 salesReport.espressoSold,
                 salesReport.espressoRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "LATTE SOLD=%u REV=%lu DKK\r\n",
                 salesReport.latteSold,
                 salesReport.latteRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "FILTER CL=%u REV=%lu DKK\r\n",
                 salesReport.filterClSold,
                 salesReport.filterRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "CASH TOTAL=%lu DKK\r\n",
                 salesReport.cashTotal);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "CARD TOTAL=%lu DKK\r\n",
                 salesReport.cardTotal);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        snprintf(msg, UART_MSG_SIZE, "OPERATING TIME=%02u:%02u:%02u\r\n",
                 systemTime.hours,
                 systemTime.minutes,
                 systemTime.seconds);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));

        command_handled = 1U;
    }
    else if (strncmp(cmd, "BUY ESPRESSO CARD ", 18U) == 0)
    {
        taskENTER_CRITICAL();
        value = prices.espresso_cup_dkk;
        taskEXIT_CRITICAL();

        logger_create_sale(PRODUCT_ESPRESSO,
                           1U,
                           value,
                           PAYMENT_CARD,
                           &cmd[18]);

        command_handled = 1U;
    }
    else if (strcmp(cmd, "BUY ESPRESSO") == 0)
    {
        taskENTER_CRITICAL();
        value = prices.espresso_cup_dkk;
        taskEXIT_CRITICAL();

        logger_create_sale(PRODUCT_ESPRESSO,
                           1U,
                           value,
                           PAYMENT_CASH,
                           "CASH");

        command_handled = 1U;
    }
    else if (strncmp(cmd, "SET TIME ", 9U) == 0)
    {
        if ((sscanf(&cmd[9], "%d:%d:%d", &hours, &minutes, &seconds) == 3) &&
            (hours >= 0) && (hours < 24) &&
            (minutes >= 0) && (minutes < 60) &&
            (seconds >= 0) && (seconds < 60))
        {
            taskENTER_CRITICAL();
            timeOfDay.hours = hours;
            timeOfDay.minutes = minutes;
            timeOfDay.seconds = seconds;
            taskEXIT_CRITICAL();

            snprintf(msg, UART_MSG_SIZE,
                     "Time of day set to %02d:%02d:%02d\r\n",
                     hours,
                     minutes,
                     seconds);
        }
        else
        {
            snprintf(msg, UART_MSG_SIZE,
                     "Invalid time. Use SET TIME HH:MM:SS\r\n");
        }

        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(UART_QUEUE_WAIT_MS));
        command_handled = 1U;
    }
    else if (strncmp(cmd, "SET ESPRESSO ", 13U) == 0)
    {
        value = atoi(&cmd[13]);

        taskENTER_CRITICAL();
        prices.espresso_cup_dkk = value;
        taskEXIT_CRITICAL();

        snprintf(msg, UART_MSG_SIZE,
                 "Espresso price set to %u DKK\r\n",
                 prices.espresso_cup_dkk);
        xQueueSend(uartQueue, &msg, portMAX_DELAY);

        command_handled = 1U;
    }
    else if (strncmp(cmd, "SET LATTE ", 10U) == 0)
    {
        value = atoi(&cmd[10]);

        taskENTER_CRITICAL();
        prices.latte_cup_dkk = value;
        taskEXIT_CRITICAL();

        snprintf(msg, UART_MSG_SIZE,
                 "Latte price set to %u DKK\r\n",
                 prices.latte_cup_dkk);
        xQueueSend(uartQueue, &msg, portMAX_DELAY);

        command_handled = 1U;
    }
    else if (strncmp(cmd, "SET FILTER ", 11U) == 0)
    {
        value = atoi(&cmd[11]);

        taskENTER_CRITICAL();
        prices.filter_dkk_per_cl = value;
        taskEXIT_CRITICAL();

        snprintf(msg, UART_MSG_SIZE,
                 "Filter price set to %u DKK/cl\r\n",
                 prices.filter_dkk_per_cl);
        xQueueSend(uartQueue, &msg, portMAX_DELAY);

        command_handled = 1U;
    }
    else
    {
    }

    if (command_handled == 0U)
    {
        snprintf(msg, UART_MSG_SIZE, "Unknown command: %s\r\n", cmd);
        xQueueSend(uartQueue, &msg, portMAX_DELAY);
    }
}

/****************************** End Of Function ****************************/

void Command_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Reads UART input and sends complete commands to parser
******************************************************************************/
{
    char command_buffer[CMD_BUFFER_SIZE];
    uint8_t buffer_index = 0U;
    char received_char;

    (void)pvParameters;

    while (1)
    {
        if (UART0_CharAvailable() != 0)
        {
            received_char = UART0_GetChar();

            if ((received_char == '\r') || (received_char == '\n'))
            {
                command_buffer[buffer_index] = '\0';

                if (buffer_index > 0U)
                {
                    ParseCommand(command_buffer);
                }

                buffer_index = 0U;
            }
            else
            {
                if (buffer_index < CMD_LAST_INDEX)
                {
                    command_buffer[buffer_index] = received_char;
                    buffer_index++;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(COMMAND_TASK_DELAY_MS));
    }
}

/****************************** End Of Function ****************************/

void Logger_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Receives log entries, updates report and sends log text to UART
******************************************************************************/
{
    LogEntry entry;
    char buffer[UART_MSG_SIZE];

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(logQueue, &entry, portMAX_DELAY) == pdTRUE)
        {
            UpdateSalesReport(&entry);

            snprintf(buffer, UART_MSG_SIZE,
                     "LOG: TIME=%02u:%02u:%02u PRODUCT=%s PRICE=%u AMOUNT=%u PAYMENT=%s INFO=%s\r\n",
                     entry.hours,
                     entry.minutes,
                     entry.seconds,
                     ProductToString(entry.product),
                     entry.price,
                     entry.amount,
                     PaymentToString(entry.payment),
                     entry.paymentInfo);

            xQueueSend(uartQueue, &buffer, portMAX_DELAY);
        }
    }
}

/****************************** End Of Function ****************************/

void Clock_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Updates operating time and time of day once per second
******************************************************************************/
{
    (void)pvParameters;

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(CLOCK_TASK_DELAY_MS));

        taskENTER_CRITICAL();

        logger_increment_time((TimeOfDay_t *)&systemTime);
        logger_increment_time(&timeOfDay);

        taskEXIT_CRITICAL();
    }
}

/****************************** End Of Function ****************************/

void UART0_Init(void)
/*****************************************************************************
*   Input    : -
*   Output   : -
*   Function : Initializes UART0 on PA0 and PA1
******************************************************************************/
{
    SYSCTL_RCGCUART_R |= UART0_CLOCK_ENABLE;
    SYSCTL_RCGCGPIO_R |= GPIO_PORT_A_ENABLE;

    while ((SYSCTL_PRGPIO_R & GPIO_PORT_A_ENABLE) == 0U)
    {
    }

    GPIO_PORTA_AFSEL_R |= GPIO_PA0_PA1_MASK;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & GPIO_PCTL_PA0_PA1_MASK) +
                        GPIO_PCTL_UART0_PA0_PA1;
    GPIO_PORTA_DEN_R |= GPIO_PA0_PA1_MASK;

    UART0_CTL_R &= ~UART0_DISABLE;
    UART0_IBRD_R = UART0_BAUD_INT;
    UART0_FBRD_R = UART0_BAUD_FRAC;
    UART0_LCRH_R = UART0_LINE_CONTROL;
    UART0_CTL_R = UART0_ENABLE;
}

/****************************** End Of Function ****************************/

void UART0_SendChar(char character)
/*****************************************************************************
*   Input    : Character to send
*   Output   : -
*   Function : Sends one character through UART0
******************************************************************************/
{
    while ((UART0_FR_R & UART0_TX_FULL) != 0U)
    {
    }

    UART0_DR_R = character;
}

/****************************** End Of Function ****************************/

void UART0_SendString(char *string)
/*****************************************************************************
*   Input    : Pointer to string
*   Output   : -
*   Function : Sends a zero-terminated string through UART0
******************************************************************************/
{
    while (*string != '\0')
    {
        UART0_SendChar(*string);
        string++;
    }
}

/****************************** End Of Function ****************************/

void UART_Task(void *pvParameters)
/*****************************************************************************
*   Input    : FreeRTOS task parameter
*   Output   : -
*   Function : Sends queued UART messages through UART0
******************************************************************************/
{
    char msg[UART_MSG_SIZE];

    (void)pvParameters;

    while (1)
    {
        if (xQueueReceive(uartQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            UART0_SendString(msg);
        }
    }
}

/****************************** End Of Function ****************************/

void UpdateSalesReport(LogEntry *entry)
/*****************************************************************************
*   Input    : Pointer to log entry
*   Output   : -
*   Function : Updates sales report totals from a log entry
******************************************************************************/
{
    taskENTER_CRITICAL();

    switch (entry->product)
    {
        case PRODUCT_ESPRESSO:
            salesReport.espressoSold++;
            salesReport.espressoRevenue += entry->price;
            break;

        case PRODUCT_LATTE:
            salesReport.latteSold++;
            salesReport.latteRevenue += entry->price;
            break;

        case PRODUCT_FILTER:
            salesReport.filterClSold += entry->amount;
            salesReport.filterRevenue += entry->price;
            break;

        default:
            break;
    }

    if (entry->payment == PAYMENT_CASH)
    {
        salesReport.cashTotal += entry->price;
    }
    else if (entry->payment == PAYMENT_CARD)
    {
        salesReport.cardTotal += entry->price;
    }
    else
    {
    }

    taskEXIT_CRITICAL();
}

/****************************** End Of Function ****************************/

#ifndef LOGGER_MAIN_H
#define LOGGER_MAIN_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "app_types.h"

#define UART_MSG_SIZE 120
#define LOG_QUEUE_LENGTH 10U
#define UART_QUEUE_LENGTH 10U

#define UART_TASK_STACK_SIZE     128U
#define LOGGER_TASK_STACK_SIZE   256U
#define CLOCK_TASK_STACK_SIZE    128U
#define COMMAND_TASK_STACK_SIZE  256U

#define UART_TASK_PRIORITY       1U
#define LOGGER_TASK_PRIORITY     1U
#define CLOCK_TASK_PRIORITY      1U
#define COMMAND_TASK_PRIORITY    1U

void UART0_SendChar(char c);
void UART0_SendString(char *str);
int UART0_CharAvailable(void);
char UART0_GetChar(void);
void ParseCommand(char *cmd);
void UART0_Init(void);
void UART_Task(void *pvParameters);
void Logger_Task(void *pvParameters);
void Clock_Task(void *pvParameters);
void Command_Task(void *pvParameters);


typedef struct
{
    Product_t product;
    uint16_t price;
    uint16_t amount;
    Payment_t payment;
    char paymentInfo[24];

    unsigned int hours;
    unsigned int minutes;
    unsigned int seconds;
} LogEntry;


typedef struct
{
    uint16_t espressoSold;
    uint16_t latteSold;
    uint16_t filterClSold;

    uint32_t espressoRevenue;
    uint32_t latteRevenue;
    uint32_t filterRevenue;

    uint32_t cashTotal;
    uint32_t cardTotal;
} SalesReport_t;

typedef struct
{
    unsigned int hours;
    unsigned int minutes;
    unsigned int seconds;
} SystemTime;

typedef struct
{
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
} TimeOfDay_t;

void UpdateSalesReport(LogEntry *entry);

extern ProductPrices_t prices;
extern SalesReport_t salesReport;
extern SystemTime systemTime;
extern TimeOfDay_t timeOfDay;
extern QueueHandle_t uartQueue;
extern QueueHandle_t logQueue;

#endif

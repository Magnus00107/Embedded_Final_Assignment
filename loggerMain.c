#include "loggerMain.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tm4c123gh6pm.h"
#include "queue.h"

#include <stdio.h>
#include <string.h>
#include "app_types.h"
#include <stdlib.h>
#include <stdint.h>

#define UART_MSG_SIZE 120
#define CMD_BUFFER_SIZE 50

QueueHandle_t uartQueue = NULL;
QueueHandle_t logQueue  = NULL;
ProductPrices_t prices = {15, 27, 3};
SalesReport_t salesReport = {0};
SystemTime systemTime = {0, 0, 0};
TimeOfDay_t timeOfDay = {0, 0, 0};

const char* ProductToString(Product_t product)
{
    switch(product)
    {
        case PRODUCT_ESPRESSO: return "ESPRESSO";
        case PRODUCT_LATTE:    return "LATTE";
        case PRODUCT_FILTER:   return "FILTER";
        default:               return "UNKNOWN";
    }
}

const char* PaymentToString(Payment_t payment)
{
    switch(payment)
    {
        case PAYMENT_CASH: return "CASH";
        case PAYMENT_CARD: return "CARD";
        default:           return "NONE";
    }
}

int UART0_CharAvailable(void)
{
    return ((UART0_FR_R & 0x10) == 0);
}

char UART0_GetChar(void)
{
    while((UART0_FR_R & 0x10) != 0);
    return (char)(UART0_DR_R & 0xFF);
}

void ParseCommand(char *cmd)
{
    char msg[UART_MSG_SIZE];
    int value;

    if(strcmp(cmd, "REPORT") == 0)
    {
        snprintf(msg, UART_MSG_SIZE, "REPORT:\r\n");
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "ESPRESSO SOLD=%u REV=%lu DKK\r\n", salesReport.espressoSold, salesReport.espressoRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "LATTE SOLD=%u REV=%lu DKK\r\n", salesReport.latteSold, salesReport.latteRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "FILTER CL=%u REV=%lu DKK\r\n", salesReport.filterClSold, salesReport.filterRevenue);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "CASH TOTAL=%lu DKK\r\n", salesReport.cashTotal);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "CARD TOTAL=%lu DKK\r\n", salesReport.cardTotal);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        snprintf(msg, UART_MSG_SIZE, "OPERATING TIME=%02u:%02u:%02u\r\n",
                systemTime.hours, systemTime.minutes, systemTime.seconds);
        xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));

        return;
    }
    if(strncmp(cmd, "BUY ESPRESSO CARD ", 18) == 0)
    {
        LogEntry entry;

        entry.product = PRODUCT_ESPRESSO;
        entry.amount  = 1;
        taskENTER_CRITICAL();
        entry.price   = prices.espresso_cup_dkk;
        taskEXIT_CRITICAL();
        entry.payment = PAYMENT_CARD;

        strncpy(entry.paymentInfo, &cmd[18], sizeof(entry.paymentInfo) - 1);
        entry.paymentInfo[sizeof(entry.paymentInfo) - 1] = '\0';

        taskENTER_CRITICAL();
        entry.hours   = timeOfDay.hours;
        entry.minutes = timeOfDay.minutes;
        entry.seconds = timeOfDay.seconds;
        taskEXIT_CRITICAL();

        xQueueSend(logQueue, &entry, portMAX_DELAY);

        return;
    }
    // BUY ESPRESSO
       if(strcmp(cmd, "BUY ESPRESSO") == 0)
       {
           LogEntry entry;

           entry.product = PRODUCT_ESPRESSO;
           entry.amount  = 1;
           taskENTER_CRITICAL();
           entry.price   = prices.espresso_cup_dkk;
           taskEXIT_CRITICAL();
           entry.payment = PAYMENT_CASH;

           strcpy(entry.paymentInfo, "CASH");

           taskENTER_CRITICAL();
           entry.hours   = timeOfDay.hours;
           entry.minutes = timeOfDay.minutes;
           entry.seconds = timeOfDay.seconds;
           taskEXIT_CRITICAL();

           xQueueSend(logQueue, &entry, portMAX_DELAY);

           return;
       }

       if(strncmp(cmd, "SET TIME ", 9) == 0)
       {
           int h, m, s;

           if(sscanf(&cmd[9], "%d:%d:%d", &h, &m, &s) == 3 &&
              h >= 0 && h < 24 &&
              m >= 0 && m < 60 &&
              s >= 0 && s < 60)
           {
               taskENTER_CRITICAL();
               timeOfDay.hours = h;
               timeOfDay.minutes = m;
               timeOfDay.seconds = s;
               taskEXIT_CRITICAL();

               snprintf(msg, UART_MSG_SIZE, "Time of day set to %02d:%02d:%02d\r\n", h, m, s);
           }
           else
           {
               snprintf(msg, UART_MSG_SIZE, "Invalid time. Use SET TIME HH:MM:SS\r\n");
           }

           xQueueSend(uartQueue, &msg, pdMS_TO_TICKS(100));
           return;
       }

       if(strncmp(cmd, "SET ESPRESSO ", 13) == 0)
       {
           value = atoi(&cmd[13]);
           taskENTER_CRITICAL();
           prices.espresso_cup_dkk = value;
           taskEXIT_CRITICAL();

           snprintf(msg, UART_MSG_SIZE, "Espresso price set to %u DKK\r\n", prices.espresso_cup_dkk);
           xQueueSend(uartQueue, &msg, portMAX_DELAY);
           return;
       }

       if(strncmp(cmd, "SET LATTE ", 10) == 0)
       {
           value = atoi(&cmd[10]);
           taskENTER_CRITICAL();
           prices.latte_cup_dkk = value;
           taskEXIT_CRITICAL();

           snprintf(msg, UART_MSG_SIZE, "Latte price set to %u DKK\r\n", prices.latte_cup_dkk);
           xQueueSend(uartQueue, &msg, portMAX_DELAY);
           return;
       }

       if(strncmp(cmd, "SET FILTER ", 11) == 0)
       {
           value = atoi(&cmd[11]);
           taskENTER_CRITICAL();
           prices.filter_dkk_per_cl = value;
           taskEXIT_CRITICAL();

           snprintf(msg, UART_MSG_SIZE, "Filter price set to %u DKK/cl\r\n", prices.filter_dkk_per_cl);
           xQueueSend(uartQueue, &msg, portMAX_DELAY);
           return;
       }
    snprintf(msg, UART_MSG_SIZE, "Unknown command: %s\r\n", cmd);
    xQueueSend(uartQueue, &msg, portMAX_DELAY);
}

void Command_Task(void *pvParameters)
{
    char cmdBuffer[50];
    int index = 0;
    char c;

    while(1)
    {
        if(UART0_CharAvailable())
        {
            c = UART0_GetChar();

            if(c == '\r' || c == '\n')
            {
                cmdBuffer[index] = '\0';

                if(index > 0)
                {
                    ParseCommand(cmdBuffer);
                }

                index = 0;
            }
            else
            {
                if(index < 49)
                {
                    cmdBuffer[index++] = c;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Logger_Task(void *pvParameters)
{
    LogEntry entry;
    char buffer[UART_MSG_SIZE];

    while(1)
    {
        if(xQueueReceive(logQueue, &entry, portMAX_DELAY) == pdTRUE)
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

void Clock_Task(void *pvParameters)
{
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));

        taskENTER_CRITICAL();

        systemTime.seconds++;

        if(systemTime.seconds >= 60)
        {
            systemTime.seconds = 0;
            systemTime.minutes++;
        }

        if(systemTime.minutes >= 60)
        {
            systemTime.minutes = 0;
            systemTime.hours++;
        }
        timeOfDay.seconds++;

        if(timeOfDay.seconds >= 60)
        {
            timeOfDay.seconds = 0;
            timeOfDay.minutes++;
        }

        if(timeOfDay.minutes >= 60)
        {
            timeOfDay.minutes = 0;
            timeOfDay.hours++;
        }

        if(timeOfDay.hours >= 24)
        {
            timeOfDay.hours = 0;
        }
        taskEXIT_CRITICAL();
    }
}

void UART0_Init(void)
{
    SYSCTL_RCGCUART_R |= 0x01; // Enable UART0
    SYSCTL_RCGCGPIO_R |= 0x01; // Enable Port A
    while((SYSCTL_PRGPIO_R & 0x01) == 0);
    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_PCTL_R = (GPIO_PORTA_PCTL_R & 0xFFFFFF00) + 0x00000011;
    GPIO_PORTA_DEN_R |= 0x03;

    UART0_CTL_R &= ~0x01;
    UART0_IBRD_R = 8; // 16 MHz / (16*(8+44/64)) = 115107 baudrate
    UART0_FBRD_R = 44; UART0_LCRH_R = 0x70; // 8-bit, FIFO
    UART0_CTL_R = 0x301; // Enable UART
}

void UART0_SendChar(char c)
{
    while((UART0_FR_R & 0x20) != 0);
    UART0_DR_R = c;
}

void UART0_SendString(char *str)
{
    while(*str)
    {
        UART0_SendChar(*str++);
    }
}

void UART_Task(void *pvParameters)
{
    char msg[UART_MSG_SIZE];

    while(1)
    {
        if(xQueueReceive(uartQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            UART0_SendString(msg);
        }
    }
}

void UpdateSalesReport(LogEntry *entry)
{
    taskENTER_CRITICAL();
    switch(entry->product)
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

    if(entry->payment == PAYMENT_CASH)
    {
        salesReport.cashTotal += entry->price;
    }
    else if(entry->payment == PAYMENT_CARD)
    {
        salesReport.cardTotal += entry->price;
    }
    taskEXIT_CRITICAL();
}

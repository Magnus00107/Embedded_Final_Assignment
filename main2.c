#include <stdint.h>
#include "tm4c123gh6pm.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ================= LCD ================= */

#define LCD_RS  (1U << 2)   // PD2
#define LCD_E   (1U << 3)   // PD3

#define LCD_D4  (1U << 4)   // PC4
#define LCD_D5  (1U << 5)   // PC5
#define LCD_D6  (1U << 6)   // PC6
#define LCD_D7  (1U << 7)   // PC7
#define LCD_DATA_MASK (LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7)

/* ================= KEYPAD =================
   Columns: PA2, PA3, PA4
   Rows:    PE0, PE1, PE2, PE3
*/

#define COL1 (1U << 2)   // PA2
#define COL2 (1U << 3)   // PA3
#define COL3 (1U << 4)   // PA4
#define COL_MASK (COL1 | COL2 | COL3)

#define ROW1 (1U << 0)   // PE0
#define ROW2 (1U << 1)   // PE1
#define ROW3 (1U << 2)   // PE2
#define ROW4 (1U << 3)   // PE3
#define ROW_MASK (ROW1 | ROW2 | ROW3 | ROW4)

/* ================= FreeRTOS ================= */

static QueueHandle_t xKeyQueue = NULL;

/* ================= Delay ================= */

static void delay_cycles(volatile uint32_t n)
{
    while (n--) { }
}

/* ================= LCD helpers ================= */

static void lcd_pulse_enable(void)
{
    GPIO_PORTD_DATA_R |= LCD_E;
    delay_cycles(2000);
    GPIO_PORTD_DATA_R &= ~LCD_E;
    delay_cycles(2000);
}

static void lcd_write_nibble(uint8_t nibble)
{
    GPIO_PORTC_DATA_R &= ~LCD_DATA_MASK;

    if (nibble & 0x01) GPIO_PORTC_DATA_R |= LCD_D4;
    if (nibble & 0x02) GPIO_PORTC_DATA_R |= LCD_D5;
    if (nibble & 0x04) GPIO_PORTC_DATA_R |= LCD_D6;
    if (nibble & 0x08) GPIO_PORTC_DATA_R |= LCD_D7;

    lcd_pulse_enable();
}

static void lcd_write_byte(uint8_t rs, uint8_t value)
{
    if (rs)
        GPIO_PORTD_DATA_R |= LCD_RS;
    else
        GPIO_PORTD_DATA_R &= ~LCD_RS;

    lcd_write_nibble((value >> 4) & 0x0F);
    lcd_write_nibble(value & 0x0F);

    delay_cycles(10000);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_write_byte(0, cmd);
}

static void lcd_data(uint8_t data)
{
    lcd_write_byte(1, data);
}

static void lcd_init_gpio(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 2) | (1U << 3);   // Port C, D
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    GPIO_PORTC_DIR_R |= LCD_DATA_MASK;
    GPIO_PORTC_DEN_R |= LCD_DATA_MASK;

    GPIO_PORTD_DIR_R |= (LCD_RS | LCD_E);
    GPIO_PORTD_DEN_R |= (LCD_RS | LCD_E);

    GPIO_PORTC_DATA_R &= ~LCD_DATA_MASK;
    GPIO_PORTD_DATA_R &= ~(LCD_RS | LCD_E);
}

static void lcd_init(void)
{
    delay_cycles(200000);

    GPIO_PORTD_DATA_R &= ~LCD_RS;

    lcd_write_nibble(0x03);
    delay_cycles(80000);

    lcd_write_nibble(0x03);
    delay_cycles(20000);

    lcd_write_nibble(0x03);
    delay_cycles(20000);

    lcd_write_nibble(0x02);   // 4-bit mode
    delay_cycles(20000);

    lcd_cmd(0x28);   // 4-bit, 2-line, 5x8
    lcd_cmd(0x08);   // display off
    lcd_cmd(0x01);   // clear
    delay_cycles(80000);
    lcd_cmd(0x06);   // entry mode
    lcd_cmd(0x0C);   // display on
}

static void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_cmd(addr);
}

static void lcd_print(const char *s)
{
    while (*s)
    {
        lcd_data((uint8_t)*s++);
    }
}

static void lcd_clear_line(uint8_t row)
{
    lcd_set_cursor(row, 0);
    lcd_print("                ");
}

/* ================= Keypad helpers ================= */

static void keypad_init(void)
{
    volatile uint32_t dummy;

    SYSCTL_RCGCGPIO_R |= (1U << 0) | (1U << 4);   // Port A, E
    dummy = SYSCTL_RCGCGPIO_R;
    (void)dummy;

    /* Columns as outputs */
    GPIO_PORTA_DIR_R |= COL_MASK;
    GPIO_PORTA_DEN_R |= COL_MASK;
    GPIO_PORTA_DATA_R &= ~COL_MASK;

    /* Rows as inputs */
    GPIO_PORTE_DIR_R &= ~ROW_MASK;
    GPIO_PORTE_DEN_R |= ROW_MASK;
}

static char keypad_scan_once(void)
{
    static const char keymap[4][3] = {
        {'#', '0', '*'},
        {'9', '8', '7'},
        {'6', '5', '4'},
        {'3', '2', '1'}
    };

    const uint8_t cols[3] = { COL1, COL2, COL3 };
    uint8_t c;

    for (c = 0; c < 3; c++)
    {
        uint32_t rows;

        GPIO_PORTA_DATA_R &= ~COL_MASK;
        GPIO_PORTA_DATA_R |= cols[c];

        delay_cycles(2000);

        rows = GPIO_PORTE_DATA_R & ROW_MASK;

        if (rows & ROW1) return keymap[0][c];
        if (rows & ROW2) return keymap[1][c];
        if (rows & ROW3) return keymap[2][c];
        if (rows & ROW4) return keymap[3][c];
    }

    return 0;
}

/* ================= FreeRTOS Tasks ================= */

static void KeypadTask(void *pvParameters)
{
    char key;
    char lastKey = 0;

    (void)pvParameters;

    keypad_init();

    while (1)
    {
        key = keypad_scan_once();

        /* detect new press only */
        if ((key != 0) && (lastKey == 0))
        {
            xQueueSend(xKeyQueue, &key, portMAX_DELAY);
        }

        lastKey = key;

        vTaskDelay(pdMS_TO_TICKS(50));   // scan/debounce period
    }
}

static void LcdTask(void *pvParameters)
{
    char key;

    (void)pvParameters;

    lcd_init_gpio();
    lcd_init();

    lcd_set_cursor(0, 0);
    lcd_print("Press key:");
    lcd_clear_line(1);

    while (1)
    {
        if (xQueueReceive(xKeyQueue, &key, portMAX_DELAY) == pdPASS)
        {
            lcd_clear_line(1);
            lcd_set_cursor(1, 0);
            lcd_print("Key: ");
            lcd_data((uint8_t)key);
        }
    }
}

/* ================= Main ================= */

int main(void)
{
    xKeyQueue = xQueueCreate(8, sizeof(char));

    if (xKeyQueue == 0)
    {
        while (1) { }
    }

    xTaskCreate(KeypadTask, "KEYPAD", 256, NULL, 2, NULL);
    xTaskCreate(LcdTask,    "LCD",    256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1)
    {
    }
}

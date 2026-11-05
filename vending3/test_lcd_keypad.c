#include <LPC17xx.h>
#include <stdio.h>
#include "LCD.h"

#define ROW_MASK (0xFUL << 10)  /* P2.10..P2.13 */
#define COL_MASK (0xFUL << 23)  /* P1.23..P1.26 */

void delay_ms(unsigned int ms)
{
    while (ms--) {
        SysTick->LOAD = SystemCoreClock / 1000UL - 1UL;
        SysTick->VAL = 0;
        SysTick->CTRL = 5UL;
        while (!(SysTick->CTRL & (1UL << 16))) { }
        SysTick->CTRL = 0;
    }
}

int main(void)
{
    unsigned int row, cols;
    char line[17];

    SystemInit();
    SystemCoreClockUpdate();

    /* Test 1: LED P0.4 on = code is running */
    LPC_PINCON->PINSEL0 &= ~(3UL << 8);
    LPC_GPIO0->FIODIR |= (1UL << 4);
    LPC_GPIO0->FIOSET = (1UL << 4);

    /* Keypad pins as GPIO */
    LPC_PINCON->PINSEL3 &= ~(0xFFUL << 14);
    LPC_PINCON->PINSEL4 &= ~(0xFFUL << 20);
    LPC_GPIO2->FIODIR |= ROW_MASK;
    LPC_GPIO1->FIODIR &= ~COL_MASK;

    /* LCD pins P0.23..P0.28 as GPIO */
    LPC_PINCON->PINSEL1 &= ~(0xFFFUL << 14);

    /* Test 2: LCD */
    lcd_init();
    lcd_line(0, "LCD OK");
    lcd_line(1, "Press a key");

    /* Test 3: keypad, shows raw row number and column bits (hex) */
    for (;;) {
        for (row = 0; row < 4; ++row) {
            LPC_GPIO2->FIOCLR = ROW_MASK;
            LPC_GPIO2->FIOSET = 1UL << (10 + row);
            delay_ms(2);
            cols = (LPC_GPIO1->FIOPIN & COL_MASK) >> 23;
            if (cols) {
                sprintf(line, "row%u cols=%X", row + 1, cols);
                lcd_line(1, line);
            }
        }
    }
}

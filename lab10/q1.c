
#include <LPC17xx.h>
#include <stdio.h>
#include "LCD.h"

unsigned long adc4, adc5, diff;
unsigned int i;
unsigned char result[17];

unsigned long read_adc(unsigned int channel)
{
    unsigned long value;

    LPC_ADC->ADCR = (1U << channel) | (9U << 8) |
                    (1U << 21) | (1U << 24);

    while ((LPC_ADC->ADGDR & (1U << 31)) == 0);

    value = (LPC_ADC->ADGDR >> 4) & 0xFFF;

    return value;
}

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    LPC_SC->PCONP |= (1U << 15);
    LPC_SC->PCONP |= (1U << 12);

    LPC_PINCON->PINSEL3 &= ~(0xFU << 28);
    LPC_PINCON->PINSEL3 |= (0xFU << 28);

    lcd_init();

    temp1 = 0x80;
    lcd_com();
    lcd_puts("ADC4-ADC5:");

    while (1)
    {
        adc4 = read_adc(4);
        adc5 = read_adc(5);

        if (adc4 >= adc5)
            diff = adc4 - adc5;
        else
            diff = adc5 - adc4;

        sprintf((char *)result, "Diff: %lu    ", diff);

        temp1 = 0xC0;
        lcd_com();
        lcd_puts(result);

        for (i = 0; i < 200000; i++);
    }
}
#include <LPC17xx.h>
#include "LCD.h"

#define COL_MASK 0x000000F0   /* P0.4-P0.7 */
#define ROW0     0x00000100   /* P0.8 */

unsigned char msg10[] = "Intensity: 10%  ";
unsigned char msg25[] = "Intensity: 25%  ";
unsigned char msg50[] = "Intensity: 50%  ";
unsigned char msg75[] = "Intensity: 75%  ";

void delay_key(void)
{
    volatile unsigned int i;
    for (i = 0; i < 50000; i++);
}

void show(unsigned char *message)
{
    temp1 = 0x80;            /* Start of LCD first line */
    lcd_com();
    lcd_puts(message);
    LPC_GPIO0->FIOSET = ROW0; /* LCD.h clears P0.8 while writing */
}

void pwm_init(void)
{
    LPC_SC->PCONP |= (1 << 6);

    LPC_PINCON->PINSEL3 &= ~(3 << 14);
    LPC_PINCON->PINSEL3 |=  (2 << 14); /* P1.23 = PWM1.4 */

    LPC_PWM1->TCR = 0x02;
    LPC_PWM1->PR  = 0;
    LPC_PWM1->MCR = 0x02;
    LPC_PWM1->PCR = (1 << 12);
    LPC_PWM1->MR0 = 1000;
    LPC_PWM1->MR4 = 100;
    LPC_PWM1->LER = (1 << 0) | (1 << 4);
    LPC_PWM1->TCR = 0x09;
}

int main(void)
{
    unsigned int key;

    SystemInit();
    SystemCoreClockUpdate();

    lcd_init();  /* Do this before configuring the keypad */

    LPC_PINCON->PINSEL0 &= ~(0x3FF << 8);
    LPC_PINCON->PINMODE0 =
        (LPC_PINCON->PINMODE0 & ~(0xFF << 8)) | (0xAA << 8);

    LPC_GPIO0->FIODIR &= ~COL_MASK;
    LPC_GPIO0->FIODIR |= ROW0;
    LPC_GPIO0->FIOSET = ROW0;

    pwm_init();
    show(msg10);

    while (1)
    {
        key = LPC_GPIO0->FIOPIN & COL_MASK;
        if (key == 0)
            continue;

        delay_key();
        key = LPC_GPIO0->FIOPIN & COL_MASK;

        if (key == (1 << 4))       /* SW3: key 0 */
        {
            LPC_PWM1->MR4 = 100;
            LPC_PWM1->LER = (1 << 4);
            show(msg10);
        }
        else if (key == (1 << 5))  /* SW4: key 1 */
        {
            LPC_PWM1->MR4 = 250;
            LPC_PWM1->LER = (1 << 4);
            show(msg25);
        }
        else if (key == (1 << 6))  /* SW5: key 2 */
        {
            LPC_PWM1->MR4 = 500;
            LPC_PWM1->LER = (1 << 4);
            show(msg50);
        }
        else if (key == (1 << 7))  /* SW6: key 3 */
        {
            LPC_PWM1->MR4 = 750;
            LPC_PWM1->LER = (1 << 4);
            show(msg75);
        }

        while (LPC_GPIO0->FIOPIN & COL_MASK);
        delay_key();
    }
}
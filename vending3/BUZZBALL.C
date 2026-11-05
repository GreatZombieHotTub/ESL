#include <LPC17xx.h>

/* Buzzer test: onboard BZ1 on P0.5 toggles at about 2 kHz, forever.
   Also blinks LED P0.4 so you know the code is running. */
#define BUZ (1UL << 5)

static void delay_us(unsigned int us)
{
    SysTick->LOAD = (SystemCoreClock / 1000000UL) * us - 1UL;
    SysTick->VAL = 0;
    SysTick->CTRL = 5UL;
    while (!(SysTick->CTRL & (1UL << 16))) { }
    SysTick->CTRL = 0;
}

int main(void)
{
    unsigned int n = 0;
    SystemInit();
    SystemCoreClockUpdate();
    LPC_PINCON->PINSEL0 &= ~(3UL << 10);   /* P0.5 GPIO */
    LPC_PINCON->PINSEL0 &= ~(3UL << 8);    /* P0.4 GPIO  */
    LPC_GPIO0->FIODIR |= BUZ | (1UL << 4);
    for (;;) {
        LPC_GPIO0->FIOSET = BUZ;
        delay_us(250);
        LPC_GPIO0->FIOCLR = BUZ;
        delay_us(250);
        if (++n >= 1000) {                 /* about every 0.5 s */
            n = 0;
            LPC_GPIO0->FIOPIN ^= (1UL << 4);
        }
    }
}
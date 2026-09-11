#include <LPC17xx.h>

void delay(void)
{
    int i;
    for(i = 0; i < 700000; i++);
}

int main(void)
{
    unsigned int count;

    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL0 &= 0xFF0000FF;
    LPC_GPIO0->FIODIR |= 0x00000FF0;

    while(1)
    {
        for(count = 255; count >=0 ; count--)
        {
            LPC_GPIO0->FIOPIN = count << 4;
            delay();
        }
    }
}
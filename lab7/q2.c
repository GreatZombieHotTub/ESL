#include <LPC17xx.h>

void delay(void)
{
    int i;
    for(i = 0; i < 5000000; i++);
}

int main(void)
{
    unsigned int count = 0;

    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL0 &= 0xFF0000FF;
    LPC_PINCON->PINSEL4 &= ~(3 << 24);

    LPC_GPIO0->FIODIR |= 0x00000FF0;
    LPC_GPIO2->FIODIR &= ~(1 << 12);

    while(1)
    {
        LPC_GPIO0->FIOPIN = count << 4;
        delay();

        if(LPC_GPIO2->FIOPIN & (1 << 12))
        {
            count++;

            if(count > 255)
                count = 0;
        }
        else
        {
            if(count == 0)
                count = 255;
            else
                count--;
        }
    }
}
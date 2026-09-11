#include <LPC17xx.h>

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL0 &= ~(3 << 8);    // P0.4 as GPIO
    LPC_PINCON->PINSEL4 &= ~(3 << 24);   // P2.12 as GPIO

    LPC_GPIO0->FIODIR |= (1 << 5);       // P0.5 as output
    LPC_GPIO2->FIODIR &= ~(1 << 12);     // P2.12 as input

    while(1)
    {
        if(!(LPC_GPIO2->FIOPIN & (1 << 12)))
        {
            LPC_GPIO0->FIOSET = (1 << 5);    // LED ON
        }
        else
        {
            LPC_GPIO0->FIOCLR = (1 << 5);    // LED OFF
        }
    }
}
#include <LPC17xx.h>

void delay(void)
{
    unsigned int i;
    for(i = 0; i < 500000; i++);
}

int main(void)
{
    LPC_GPIO0->FIODIR |= (1 << 23);

    while(1)
    {
        LPC_GPIO0->FIOSET = (1 << 23);
        delay();

        LPC_GPIO0->FIOCLR = (1 << 23);
        delay();
    }
}
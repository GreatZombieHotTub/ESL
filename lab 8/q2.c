#include <LPC17xx.h>

#define FIRST_SEG   0xF87FFFFF
#define SECOND_SEG  0xF8FFFFFF
#define THIRD_SEG   0xF97FFFFF
#define FOURTH_SEG  0xF9FFFFFF

unsigned char seven_seg[] =
{
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void delay(void)
{
    unsigned int i;
    for(i = 0; i < 5000000; i++);
}

void display(int number)
{
    int d1, d2, d3, d4;

    d1 = number % 10;
    d2 = (number / 10) % 10;
    d3 = (number / 100) % 10;
    d4 = (number / 1000) % 10;

    LPC_GPIO1->FIOPIN = FIRST_SEG;
    LPC_GPIO0->FIOPIN = seven_seg[d1] << 4;
    delay();

    LPC_GPIO1->FIOPIN = SECOND_SEG;
    LPC_GPIO0->FIOPIN = seven_seg[d2] << 4;
    delay();

    LPC_GPIO1->FIOPIN = THIRD_SEG;
    LPC_GPIO0->FIOPIN = seven_seg[d3] << 4;
    delay();

    LPC_GPIO1->FIOPIN = FOURTH_SEG;
    LPC_GPIO0->FIOPIN = seven_seg[d4] << 4;
    delay();
}

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL0 &= 0xFF0000FF;
    LPC_PINCON->PINSEL3 &= 0xFFC03FFF;

    LPC_GPIO0->FIODIR |= 0x00000FF0;
    LPC_GPIO1->FIODIR |= 0x07800000;

    while(1)
    {
        display(1);
        delay();

        display(10);
        delay();

        display(100);
        delay();

        display(1000);
        delay();
    }
}
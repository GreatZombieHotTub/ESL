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

int d1 = 0;
int d2 = 0;
int d3 = 0;
int d4 = 0;

void delay(void)
{
    unsigned int i;
    for(i = 0; i < 2000; i++);
}

void display(void)
{
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

void count_up(void)
{
    d1++;

    if(d1 == 10)
    {
        d1 = 0;
        d2++;

        if(d2 == 10)
        {
            d2 = 0;
            d3++;

            if(d3 == 10)
            {
                d3 = 0;
                d4++;

                if(d4 == 10)
                    d4 = 0;
            }
        }
    }
}

void count_down(void)
{
    if(d1 > 0)
    {
        d1--;
    }
    else
    {
        d1 = 9;

        if(d2 > 0)
        {
            d2--;
        }
        else
        {
            d2 = 9;

            if(d3 > 0)
            {
                d3--;
            }
            else
            {
                d3 = 9;

                if(d4 > 0)
                    d4--;
                else
                    d4 = 9;
            }
        }
    }
}

void one_second(void)
{
    unsigned int i;

    for(i = 0; i < 250; i++)
    {
        display();
    }
}

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    /* P0.4-P0.11 as GPIO */
    LPC_PINCON->PINSEL0 &= 0xFF0000FF;

    /* P1.23-P1.26 as GPIO */
    LPC_PINCON->PINSEL3 &= 0xFFC03FFF;

    /* P2.0 as GPIO */
    LPC_PINCON->PINSEL4 &= ~(3 << 0);

    /* Display outputs */
    LPC_GPIO0->FIODIR |= 0x00000FF0;
    LPC_GPIO1->FIODIR |= 0x07800000;

    /* SW2 input at P2.0 */
    LPC_GPIO2->FIODIR &= ~(1 << 0);

    while(1)
    {
        one_second();

        if(!(LPC_GPIO2->FIOPIN & (1 << 0)))
        {
            count_down();
        }
        else
        {
            count_up();
        }
    }
}
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

unsigned int dig_count;
unsigned int ring_count = 0;
unsigned int digit_value[] = {1, 10, 100, 1000};
unsigned long select_segment[] =
{
    FIRST_SEG, SECOND_SEG, THIRD_SEG, FOURTH_SEG
};

void Display(void);
void delay(void);

int main(void)
{
    unsigned int i;

    SystemInit();
    SystemCoreClockUpdate();

    LPC_PINCON->PINSEL0 &= 0xFF0000FF;
    LPC_PINCON->PINSEL3 &= 0xFFC03FFF;

    LPC_GPIO0->FIODIR |= 0x00000FF0;
    LPC_GPIO1->FIODIR |= 0x07800000;

    while(1)
    {
        for(i = 0; i < 250; i++)
            Display();

        ring_count++;

        if(ring_count == 4)
            ring_count = 0;
    }
}

void Display(void)
{
    unsigned int number = digit_value[ring_count];

    for(dig_count = 0; dig_count < 4; dig_count++)
    {
        LPC_GPIO1->FIOPIN = select_segment[dig_count];
        LPC_GPIO0->FIOPIN = seven_seg[number % 10] << 4;
        delay();
        number /= 10;
    }
}

void delay(void)
{
    volatile unsigned int i;

    for(i = 0; i < 2000; i++);
}

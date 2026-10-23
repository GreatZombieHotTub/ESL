
#include <LPC17xx.h>

#define RS_CTRL (1U << 27)
#define EN_CTRL (1U << 28)
#define DT_CTRL (0x0FU << 23)
#define SW2_PIN (1U << 12)

unsigned long int temp1, temp2;
unsigned char flag1;
unsigned int i, seed = 1234, dice;

unsigned long int init_command[] =
{0x30, 0x30, 0x30, 0x20, 0x28, 0x0C, 0x06, 0x01, 0x80};

void lcd_write(void);
void port_write(void);
void delay_lcd(unsigned int r1);
void lcd_string(unsigned char *msg);
void lcd_number(unsigned int num);

int main(void)
{
    SystemInit();
    SystemCoreClockUpdate();

    LPC_GPIO0->FIODIR |= DT_CTRL | RS_CTRL | EN_CTRL;
    LPC_GPIO2->FIODIR &= ~SW2_PIN;

    flag1 = 0;

    for (i = 0; i < 9; i++)
    {
        temp1 = init_command[i];
        lcd_write();
    }

    flag1 = 1;
    lcd_string("DICE TOSS");
    temp1 = 0xC0;
    flag1 = 0;
    lcd_write();
    flag1 = 1;
    lcd_string("Press SW2");

    while (1)
    {
        seed++;

        if ((LPC_GPIO2->FIOPIN & SW2_PIN) == 0)
        {
            delay_lcd(30000);

            if ((LPC_GPIO2->FIOPIN & SW2_PIN) == 0)
            {
                dice = (seed % 6) + 1;

                temp1 = 0x01;
                flag1 = 0;
                lcd_write();

                flag1 = 1;
                lcd_string("Dice Value: ");

                lcd_number(dice);

                while ((LPC_GPIO2->FIOPIN & SW2_PIN) == 0);
                delay_lcd(30000);
            }
        }
    }
}

void lcd_write(void)
{
    unsigned char flag2;

    flag2 = (flag1 == 1) ? 0 :
            ((temp1 == 0x30) || (temp1 == 0x20)) ? 1 : 0;

    temp2 = temp1 & 0xF0;
    temp2 <<= 19;
    port_write();

    if (flag2 == 0)
    {
        temp2 = temp1 & 0x0F;
        temp2 <<= 23;
        port_write();
    }
}

void port_write(void)
{
    LPC_GPIO0->FIOPIN = temp2;

    if (flag1 == 0)
        LPC_GPIO0->FIOCLR = RS_CTRL;
    else
        LPC_GPIO0->FIOSET = RS_CTRL;

    LPC_GPIO0->FIOSET = EN_CTRL;
    delay_lcd(25);
    LPC_GPIO0->FIOCLR = EN_CTRL;
    delay_lcd(30000);
}

void lcd_string(unsigned char *msg)
{
    while (*msg != '\0')
    {
        temp1 = *msg++;
        lcd_write();
    }
}

void lcd_number(unsigned int num)
{
    temp1 = num + '0';
    lcd_write();
}

void delay_lcd(unsigned int r1)
{
    unsigned int r;

    for (r = 0; r < r1; r++);
}
/* NIGGER CONNECT TO Connection 1: LCD

Connect a 10-core FRC cable as follows:

- CND on the controller board ? CNAD on the LCD interface.

The LCD is mounted on connector CN11.

Connection 2: Dice-toss switch (SW2)

Use the board's onboard SW2 switch.

No extra wire is needed if SW2 is already connected to the controller through the board's circuit.

Confirm its GPIO pin from the board schematic.

Connection 3: Jumpers and contrast

Short jumper JP16.

Short jumper JP5.

Adjust POT3 until the LCD characters are clearly visible.*/

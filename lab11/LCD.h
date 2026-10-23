#ifndef LCD_H
#define LCD_H

#include <LPC17xx.h>

/* LCD connections
   RS  -> P0.27
   EN  -> P0.28
   D4  -> P0.23
   D5  -> P0.24
   D6  -> P0.25
   D7  -> P0.26
*/

#define RS_CTRL  0x08000000    /* P0.27 */
#define EN_CTRL  0x10000000    /* P0.28 */
#define DT_CTRL  0x07800000    /* P0.23 - P0.26 */


/* Global variables */
unsigned long int temp1 = 0;
unsigned long int temp2 = 0;

unsigned char flag1 = 0;
unsigned char flag2 = 0;


/* Function declarations */
void lcd_init(void);
void lcd_write(void);
void lcd_com(void);
void lcd_puts(unsigned char *str);
void port_write(void);
void delay_lcd(unsigned int);


/* LCD initialization commands */
unsigned long int init_command[] =
{
    0x30,
    0x30,
    0x30,
    0x20,
    0x28,
    0x0C,
    0x06,
    0x01,
    0x80
};


/* =====================================================
   LCD INITIALIZATION
   ===================================================== */
void lcd_init(void)
{
    unsigned int i;

    /* Configure LCD pins as output */
    LPC_GPIO0->FIODIR = DT_CTRL | RS_CTRL | EN_CTRL;

    /* Initially disable RS and EN */
    LPC_GPIO0->FIOCLR = RS_CTRL | EN_CTRL;

    /* Give LCD time to power up */
    delay_lcd(100000);

    /* Command mode */
    flag1 = 0;

    /* LCD initialization */
    for (i = 0; i < 9; i++)
    {
        temp1 = init_command[i];
        lcd_write();

        /* Extra delay during initialization */
        delay_lcd(50000);
    }

    /* Extra delay after initialization */
    delay_lcd(100000);
}


/* =====================================================
   LCD WRITE
   ===================================================== */
void lcd_write(void)
{
    /*
       flag2 = 1:
       Send only upper nibble.
       
       flag2 = 0:
       Send upper nibble and lower nibble.
    */

    flag2 = (flag1 == 1) ? 0 :
            ((temp1 == 0x30) || (temp1 == 0x20)) ? 1 : 0;


    /* Send upper nibble */
    temp2 = temp1 & 0xF0;

    /* Move bits 4-7 to P0.23-P0.26 */
    temp2 = temp2 << 19;

    port_write();


    /* Send lower nibble */
    if (flag2 == 0)
    {
        temp2 = temp1 & 0x0F;

        /* Move bits 0-3 to P0.23-P0.26 */
        temp2 = temp2 << 23;

        port_write();
    }
}


/* =====================================================
   SEND COMMAND TO LCD
   ===================================================== */
void lcd_com(void)
{
    /* RS = 0 means command */
    flag1 = 0;

    lcd_write();
}


/* =====================================================
   SEND STRING TO LCD
   ===================================================== */
void lcd_puts(unsigned char *str)
{
    /* RS = 1 means data */
    flag1 = 1;

    while (*str != '\0')
    {
        temp1 = *str;

        lcd_write();

        str++;
    }
}


/* =====================================================
   SEND DATA TO LCD PORT
   ===================================================== */
void port_write(void)
{
    /* Put data on P0.23-P0.26 */
    LPC_GPIO0->FIOPIN = temp2;


    /* RS control */
    if (flag1 == 0)
    {
        /* Command mode */
        LPC_GPIO0->FIOCLR = RS_CTRL;
    }
    else
    {
        /* Data mode */
        LPC_GPIO0->FIOSET = RS_CTRL;
    }


    /* Enable pulse */
    LPC_GPIO0->FIOSET = EN_CTRL;

    delay_lcd(25);

    LPC_GPIO0->FIOCLR = EN_CTRL;

    delay_lcd(30000);
}


/* =====================================================
   LCD DELAY
   ===================================================== */
void delay_lcd(unsigned int r1)
{
    unsigned int r;

    for (r = 0; r < r1; r++);

    return;
}

#endif
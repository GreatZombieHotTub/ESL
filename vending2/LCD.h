#ifndef LCD_H
#define LCD_H

#include <LPC17xx.h>

/* 16x2 LCD, four-bit mode: RS=P0.27, EN=P0.28, D4..D7=P0.23..P0.26.
   R/W is tied low on the lab board. Call delay_ms() after SysTick setup. */
#define LCD_RS    (1UL << 27)
#define LCD_EN    (1UL << 28)
#define LCD_DATA  (0xFUL << 23)

void delay_ms(unsigned int ms);

static void lcd_nibble(unsigned char nibble)
{
    LPC_GPIO0->FIOCLR = LCD_DATA;
    LPC_GPIO0->FIOSET = ((unsigned long)(nibble & 0x0F) << 23);
    LPC_GPIO0->FIOSET = LCD_EN;
    /* GPIO writes and the next delay provide the enable pulse width. */
    delay_ms(1);
    LPC_GPIO0->FIOCLR = LCD_EN;
    delay_ms(1);
}

static void lcd_byte(unsigned char value, unsigned char data)
{
    if (data) LPC_GPIO0->FIOSET = LCD_RS;
    else      LPC_GPIO0->FIOCLR = LCD_RS;

    lcd_nibble(value >> 4);
    lcd_nibble(value);
    if (value == 0x01 || value == 0x02) delay_ms(2);
}

static void lcd_command(unsigned char command)
{
    lcd_byte(command, 0);
}

/* Write exactly 16 columns; short messages erase any previous trailing text. */
static void lcd_line(unsigned char row, const char *text)
{
    unsigned int i;
    unsigned char ended = 0;
    lcd_command(row ? 0xC0 : 0x80);
    for (i = 0; i < 16; ++i) {
        if (!ended && text[i] == '\0') ended = 1;
        lcd_byte(ended ? (unsigned char)' ' : (unsigned char)text[i], 1);
    }
}

static void lcd_init(void)
{
    LPC_GPIO0->FIODIR |= LCD_DATA | LCD_RS | LCD_EN;
    LPC_GPIO0->FIOCLR = LCD_DATA | LCD_RS | LCD_EN;
    delay_ms(40);

    LPC_GPIO0->FIOCLR = LCD_RS;
    lcd_nibble(3); delay_ms(5);
    lcd_nibble(3); delay_ms(1);
    lcd_nibble(3); delay_ms(1);
    lcd_nibble(2); /* Switch to four-bit mode. */

    lcd_command(0x28); /* 4-bit, 2 lines, 5x8 characters */
    lcd_command(0x0C); /* display on, cursor off */
    lcd_command(0x06); /* increment cursor */
    lcd_command(0x01); /* clear */
}

#endif

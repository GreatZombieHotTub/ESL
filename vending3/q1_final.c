#include <LPC17xx.h>
#include <stdio.h>
#include "LCD.h"

/* Controller: LPC1768. Keypad: CNB -> CNB3; LCD: CND -> CNAD;
   LEDs: CNA -> CNA1. P0.5 is reserved for BZ1, so use seven LEDs. */
#define ROW_MASK       (0xFUL << 10) /* P2.10..P2.13 (outputs) */
#define COL_MASK       (0xFUL << 23) /* P1.23..P1.26 (inputs)  */
#define LED_MASK       (0xFDUL << 4) /* P0.4, P0.6..P0.11      */
#define INITIAL_STOCK  5U
#define MAX_BALANCE    500U
#define MAX_ITEMS      (3U * INITIAL_STOCK)
#define ADMIN_PIN      "1234"        /* demonstration PIN */
#define MAX_PIN_FAILS  3U
#define LCD_WIDTH      16

/* BZ1 buzzer on CNC pin 1 = P0.15 (CNC pin 10 = GND). Passive buzzer plays pitches;
   an active buzzer may only beep. Set both to 255 to disable. */
#define BZ1_PORT 0
#define BZ1_PIN  15
#if BZ1_PORT == 255 && BZ1_PIN == 255
#define BZ1_ENABLED 0
#elif BZ1_PORT <= 2 && BZ1_PIN < 32
#define BZ1_ENABLED 1
#define BZ1_PIN_MASK (1UL << BZ1_PIN)
#if BZ1_PORT == 0
#define BZ1_GPIO LPC_GPIO0
#elif BZ1_PORT == 1
#define BZ1_GPIO LPC_GPIO1
#else
#define BZ1_GPIO LPC_GPIO2
#endif
#else
#error Check BZ1_PORT and BZ1_PIN against the schematic.
#endif
#if BZ1_ENABLED
#if (BZ1_PORT == 0 && (((BZ1_PIN >= 4 && BZ1_PIN <= 11) && BZ1_PIN != 5) || (BZ1_PIN >= 23 && BZ1_PIN <= 28))) || \
    (BZ1_PORT == 1 && BZ1_PIN >= 23 && BZ1_PIN <= 26) || \
    (BZ1_PORT == 2 && BZ1_PIN >= 10 && BZ1_PIN <= 13)
#error BZ1 GPIO conflicts with keypad, LCD, or progress LEDs.
#endif
#endif

typedef struct {
    const char *name;
    unsigned char price;
    unsigned char stock;
} Product;

static Product products[3] = {
    {"Water", 10, INITIAL_STOCK},
    {"Juice", 20, INITIAL_STOCK},
    {"Snack", 15, INITIAL_STOCK}
};

/* Software labels for the physical switches SW3..SW18. */
static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static unsigned int balance = 0;         /* RAM wallet: persists after purchase */
static unsigned int cart_total = 0;
static unsigned char cart_qty[3] = {0, 0, 0};
static unsigned char order[MAX_ITEMS];   /* selection order = dispense order */
static unsigned char item_count = 0;
static unsigned char failed_attempts = 0;
static unsigned char pin_fails = 0;

/* ---------- timing ---------- */

/* Polled SysTick delay. LOAD is 24-bit: fine up to ~100 ms at 100 MHz. */
static void delay_us(unsigned int us)
{
    unsigned long ticks = SystemCoreClock / 1000000UL;
    if (!us || !ticks) return;
    SysTick->LOAD = ticks * us - 1UL;
    SysTick->VAL = 0;
    SysTick->CTRL = 5UL; /* processor clock, enable, no interrupt */
    while (!(SysTick->CTRL & (1UL << 16))) { }
    SysTick->CTRL = 0;
}

void delay_ms(unsigned int ms)
{
    while (ms--) delay_us(1000);
}

/* ---------- LCD ---------- */

/* Pads every line to 16 chars so shorter text never leaves old characters. */
static void put_line(unsigned int row, const char *text)
{
    char buf[LCD_WIDTH + 1];
    unsigned int i = 0;
    while (i < LCD_WIDTH && text[i]) { buf[i] = text[i]; ++i; }
    while (i < LCD_WIDTH) buf[i++] = ' ';
    buf[LCD_WIDTH] = '\0';
    lcd_line(row, buf);
}

static void show(const char *top, const char *bottom)
{
    put_line(0, top);
    put_line(1, bottom);
}

/* ---------- keypad ---------- */

/* One row driven high at a time; columns have pull-downs. */
static char scan_key(void)
{
    unsigned int row, col, columns;
    LPC_GPIO2->FIOCLR = ROW_MASK;
    for (row = 0; row < 4; ++row) {
        LPC_GPIO2->FIOSET = 1UL << (10 + row);
        delay_ms(1);
        columns = (LPC_GPIO1->FIOPIN & COL_MASK) >> 23;
        LPC_GPIO2->FIOCLR = 1UL << (10 + row);
        for (col = 0; col < 4; ++col)
            if (columns & (1UL << col)) return keymap[row][col];
    }
    return 0;
}

/* Debounced; a held key gives exactly one event. */
static char get_key(void)
{
    char key;
    for (;;) {
        do { key = scan_key(); } while (!key);
        delay_ms(25);
        if (scan_key() == key) break;
    }
    while (scan_key()) delay_ms(10);
    delay_ms(25);
    return key;
}

/* ---------- init ---------- */

static void hardware_init(void)
{
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        LPC_PINCON->PINSEL3 &= ~(3UL << (14 + 2*i));  /* P1.23..26 GPIO */
        LPC_PINCON->PINSEL4 &= ~(3UL << (20 + 2*i));  /* P2.10..13 GPIO */
        LPC_PINCON->PINMODE3 |= (3UL << (14 + 2*i));  /* column pull-down */
    }
    for (i = 23; i <= 28; ++i)                         /* LCD pins P0.23..28 */
        LPC_PINCON->PINSEL1 &= ~(3UL << (2*(i-16)));
    LPC_PINCON->PINSEL0 &= ~0x00FFFF00UL;              /* P0.4..P0.11 GPIO */

    LPC_GPIO2->FIODIR |= ROW_MASK;
    LPC_GPIO2->FIOCLR = ROW_MASK;
    LPC_GPIO1->FIODIR &= ~COL_MASK;
    LPC_GPIO0->FIODIR |= LED_MASK;
    LPC_GPIO0->FIOCLR = LED_MASK;
#if BZ1_ENABLED
#if BZ1_PORT == 0
#if BZ1_PIN < 16
    LPC_PINCON->PINSEL0 &= ~(3UL << (2*BZ1_PIN));
#else
    LPC_PINCON->PINSEL1 &= ~(3UL << (2*(BZ1_PIN-16)));
#endif
#elif BZ1_PORT == 1
#if BZ1_PIN < 16
    LPC_PINCON->PINSEL2 &= ~(3UL << (2*BZ1_PIN));
#else
    LPC_PINCON->PINSEL3 &= ~(3UL << (2*(BZ1_PIN-16)));
#endif
#else
#if BZ1_PIN < 16
    LPC_PINCON->PINSEL4 &= ~(3UL << (2*BZ1_PIN));
#else
    LPC_PINCON->PINSEL5 &= ~(3UL << (2*(BZ1_PIN-16)));
#endif
#endif
    BZ1_GPIO->FIODIR |= BZ1_PIN_MASK;
    BZ1_GPIO->FIOCLR = BZ1_PIN_MASK;
#endif
    lcd_init();
}

/* ---------- cart and wallet ---------- */

static void cart_clear(void)
{
    unsigned int i;
    item_count = 0;
    cart_total = 0;
    failed_attempts = 0;
    for (i = 0; i < 3; ++i) cart_qty[i] = 0;
}

static void show_menu(void)
{
    char line[LCD_WIDTH + 1];
    sprintf(line, "B%u I%u D=Pay", balance, (unsigned int)item_count);
    show("1W10 2J20 3S15", line);
}

static void show_cart(void)
{
    char top[LCD_WIDTH + 1], bottom[LCD_WIDTH + 1];
    sprintf(top, "W%u J%u S%u", (unsigned int)cart_qty[0],
            (unsigned int)cart_qty[1], (unsigned int)cart_qty[2]);
    sprintf(bottom, "Tot%u Bal%u", cart_total, balance);
    show(top, bottom);
    delay_ms(1600);
}

static void add_item(unsigned int item)
{
    char top[LCD_WIDTH + 1], bottom[LCD_WIDTH + 1];
    if (!products[item].stock) {
        show("Out of stock", products[item].name);
        delay_ms(1100);
        return;
    }
    if (cart_qty[item] >= products[item].stock) {
        show("Stock limit", "Choose another");
        delay_ms(1100);
        return;
    }
    order[item_count++] = (unsigned char)item; /* <= MAX_ITEMS via stock checks */
    ++cart_qty[item];
    cart_total += products[item].price;
    sprintf(top, "%s x%u", products[item].name, (unsigned int)cart_qty[item]);
    sprintf(bottom, "Cart total Rs%u", cart_total); /* max 16 chars */
    show(top, bottom);
    delay_ms(750);
}

static void remove_last(void)
{
    unsigned int item;
    if (!item_count) {
        show("Cart is empty", "Select 1, 2 or 3");
    } else {
        item = order[--item_count];
        --cart_qty[item];
        cart_total -= products[item].price;
        show("Last item purchased", products[item].name);
    }
    delay_ms(850);
}

static void add_credit(unsigned int amount)
{
    char line[LCD_WIDTH + 1];
    if (balance + amount > MAX_BALANCE) {
        show("Max balance 500", "Pay or refund");
    } else {
        balance += amount;
        sprintf(line, "Balance Rs%u", balance);
        show("Credit added", line);
    }
    delay_ms(800);
}

/* ---------- buzzer ---------- */

#if BZ1_ENABLED
static void play_note(unsigned int hz, unsigned int duration_ms)
{
    unsigned long half_us, cycles, i;
    if (!hz) { delay_ms(duration_ms); return; }
    half_us = 500000UL / hz;
    cycles = (1000UL * duration_ms) / (2UL * half_us);
    for (i = 0; i < cycles; ++i) {
        BZ1_GPIO->FIOSET = BZ1_PIN_MASK;
        delay_us((unsigned int)half_us);
        BZ1_GPIO->FIOCLR = BZ1_PIN_MASK;
        delay_us((unsigned int)half_us);
    }
}
#endif

static void success_tune(void)
{
#if BZ1_ENABLED
    /* Notes/durations adapted from Arduino's public-domain toneMelody example:
       https://docs.arduino.cc/built-in-examples/digital/toneMelody/ */
    static const unsigned int notes[8] = {262,196,196,220,196,0,247,262};
    static const unsigned char beat[8] = {4,8,8,4,4,4,4,4};
    unsigned int i, duration;
    for (i = 0; i < 8; ++i) {
        duration = 1000U / beat[i];
        play_note(notes[i], duration);
        delay_ms(duration / 5U);
    }
#endif
}

/* ---------- checkout ---------- */

static void show_progress(unsigned int product_index, unsigned int n,
                          unsigned int total)
{
    unsigned int step;
    char top[LCD_WIDTH + 1];
    static const unsigned char progress_pins[7] = {4, 6, 7, 8, 9, 10, 11};
    sprintf(top, "Dispense %u/%u", n, total);
    show(top, products[product_index].name);
    LPC_GPIO0->FIOCLR = LED_MASK;
    for (step = 0; step < 7; ++step) {
        LPC_GPIO0->FIOSET = 1UL << progress_pins[step];
        delay_ms(140); /* growing progress bar */
    }
    LPC_GPIO0->FIOCLR = LED_MASK;
    delay_ms(120);
}

static void checkout(void)
{
    unsigned int i, total, charged;
    char line[LCD_WIDTH + 1];
    if (!item_count) {
        show("Cart is empty", "Choose 1, 2 or 3");
        delay_ms(1100);
        return;
    }
    if (balance < cart_total) {
        ++failed_attempts;
        if (failed_attempts >= 3) {
            charged = balance; /* simulated refund on cancellation */
            cart_clear();
            balance = 0;
            sprintf(line, "Refund Rs%u", charged);
            show("3 failed: Cancel", line);
            delay_ms(1800);
        } else {
            sprintf(line, "Need%u Try%u/3", cart_total - balance,
                    (unsigned int)failed_attempts);
            show("Payment failed", line);
            delay_ms(1450);
        }
        return;
    }

    charged = cart_total;
    total = item_count;
    balance -= charged;
    for (i = 0; i < 3; ++i) products[i].stock -= cart_qty[i];
    for (i = 0; i < total; ++i)
        show_progress(order[i], i + 1, total);
    cart_clear();
    success_tune();
    sprintf(line, "Balance Rs%u", balance);
    show("Purchase done!", line);
    delay_ms(1700);
}

/* ---------- admin ---------- */

static unsigned char verify_pin(void)
{
    unsigned int i;
    char entry[5] = "____", digits[4];
    char key;
    show("Admin PIN (4)", entry);
    for (i = 0; i < 4;) {
        key = get_key();
        if (key == '#') return 0;
        if (key < '0' || key > '9') continue;
        digits[i] = key;
        entry[i++] = '*';
        put_line(1, entry);
    }
    for (i = 0; i < 4; ++i) {
        if (digits[i] != ADMIN_PIN[i]) {
            show("Incorrect PIN", "Access denied");
            delay_ms(1400);
            if (++pin_fails >= MAX_PIN_FAILS) {   /* brute-force lockout */
                show("Too many tries", "Locked for 10 s");
                delay_ms(10000);
                pin_fails = 0;
            }
            return 0;
        }
    }
    pin_fails = 0;
    return 1;
}

static void admin_mode(void)
{
    char key, line[LCD_WIDTH + 1];
    unsigned int item;
    if (!verify_pin()) return;
    for (;;) {
        show("Admin: 1/2/3", "View  #=Exit");
        key = get_key();
        if (key == '#') return;
        if (key < '1' || key > '3') continue;
        item = (unsigned int)(key - '1');
        for (;;) {
            sprintf(line, "%s Stock:%u", products[item].name,
                    (unsigned int)products[item].stock);
            show(line, "*=Refill #=Back");
            key = get_key();
            if (key == '#') break;
            if (key == '*') {
                products[item].stock = INITIAL_STOCK;
                sprintf(line, "Restocked to %u", INITIAL_STOCK);
                show(line, products[item].name);
                delay_ms(1000);
            }
        }
    }
}

/* ---------- main ---------- */

int main(void)
{
    char key, line[LCD_WIDTH + 1];
    unsigned int refunded;
    SystemInit();
    SystemCoreClockUpdate();
    hardware_init();
    show("Vending machine", "Ready");
    delay_ms(1000);
    show("5/0/A = +5/10/20", "D pay C cart");
    delay_ms(1800);
    show("* undo # refund", "B = admin");
    delay_ms(1800);

    for (;;) {
        show_menu();
        key = get_key();
        if (key >= '1' && key <= '3') add_item((unsigned int)(key - '1'));
        else if (key == '5') add_credit(5);
        else if (key == '0') add_credit(10);
        else if (key == 'A') add_credit(20);
        else if (key == 'D') checkout();
        else if (key == 'C') show_cart();
        else if (key == '*') remove_last();
        else if (key == 'B') admin_mode();
        else if (key == '#') {
            refunded = balance;
            balance = 0;
            cart_clear();
            sprintf(line, "Refund Rs%u", refunded);
            show("Cart cancelled", line);
            delay_ms(1300);
        } else {
            show("Unused key", "Use 1/2/3 or C");
            delay_ms(900);
        }
    }
}
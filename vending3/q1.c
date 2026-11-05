#include <LPC17xx.h>
#include <stdio.h>
#include "LCD.h"

/* Rows: P2.10..P2.13 (outputs). Columns: P1.23..P1.26 (inputs).
   LED: P0.4, active high. These are the wiring assumptions for this build. */
#define ROW_MASK       (0xFUL << 10)
#define COL_MASK       (0xFUL << 23)
#define LED_PIN        (1UL << 4)
#define INITIAL_STOCK  5
#define MAX_CREDIT     100
#define ADMIN_PIN      "1234" /* Demo PIN; change before showing the project. */

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

static const char keymap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* SysTick supplies a stable millisecond delay after SystemCoreClockUpdate(). */
void delay_ms(unsigned int ms)
{
    while (ms--) {
        SysTick->LOAD = SystemCoreClock / 1000UL - 1UL;
        SysTick->VAL = 0;
        SysTick->CTRL = 5UL; /* processor clock, enable, no interrupt */
        while (!(SysTick->CTRL & (1UL << 16))) { }
        SysTick->CTRL = 0;
    }
}

static void show(const char *top, const char *bottom)
{
    lcd_line(0, top);
    lcd_line(1, bottom);
}

/* Scan a single key with one row high at a time (columns have pull-downs). */
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

/* A held key generates one event, including while the LCD is updating. */
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

static void hardware_init(void)
{
    unsigned int i;
    /* P1.23..26: PINSEL3; P2.10..13: PINSEL4. */
    for (i = 0; i < 4; ++i) {
        LPC_PINCON->PINSEL3 &= ~(3UL << (14 + 2*i));
        LPC_PINCON->PINSEL4 &= ~(3UL << (20 + 2*i));
        LPC_PINCON->PINMODE3 = (LPC_PINCON->PINMODE3 &
                               ~(3UL << (14 + 2*i))) |
                               (3UL << (14 + 2*i)); /* column pull-down */
    }
    /* P0.23..28 for LCD and P0.4 for dispensing LED. */
    for (i = 23; i <= 28; ++i)
        LPC_PINCON->PINSEL1 &= ~(3UL << (2*(i-16)));
    LPC_PINCON->PINSEL0 &= ~(3UL << 8);

    LPC_GPIO2->FIODIR |= ROW_MASK;
    LPC_GPIO2->FIOCLR = ROW_MASK;
    LPC_GPIO1->FIODIR &= ~COL_MASK;
    LPC_GPIO0->FIODIR |= LED_PIN;
    LPC_GPIO0->FIOCLR = LED_PIN;
    lcd_init();
}

static void menu(void)
{
    /* Select 1, 2, or 3 at either menu page. * changes page. */
    show("1 Water  Rs10", "2 Juice  Rs20");
}

static void payment_screen(unsigned int item, unsigned int credit)
{
    char line[17];
    sprintf(line, "%s Rs%u", products[item].name, products[item].price);
    lcd_line(0, line);
    if (credit >= products[item].price)
        sprintf(line, "Rs%u D=Buy #=X", credit);
    else
        sprintf(line, "Rs%u Need%u #=X", credit,
                products[item].price - credit);
    lcd_line(1, line);
}

static void purchase(unsigned int item)
{
    unsigned int credit = 0, amount;
    char key, line[17];
    Product *p = &products[item];

    if (!p->stock) {
        show("Out of stock", "Choose another");
        delay_ms(1300);
        return;
    }

    show("Add credit:", "5=5 0=10 A=20");
    delay_ms(1600);

    for (;;) {
        payment_screen(item, credit);
        key = get_key();
        if (key == '#') {
            sprintf(line, "Refund: Rs%u", credit);
            show("Cancelled", line);
            delay_ms(1500);
            return;
        }
        if (key == 'D') {
            if (credit < p->price) {
                sprintf(line, "Need Rs%u more", p->price - credit);
                show("Add more credit", line);
                delay_ms(1100);
                continue;
            }
            /* Dispensing is the only place that reduces stock. */
            p->stock--;
            LPC_GPIO0->FIOSET = LED_PIN;
            show("Dispensing...", p->name);
            delay_ms(1500);
            LPC_GPIO0->FIOCLR = LED_PIN;
            sprintf(line, "Change: Rs%u", credit - p->price);
            show("Thank you!", line);
            delay_ms(1600);
            return;
        }
        amount = key == '5' ? 5U : key == '0' ? 10U :
                 key == 'A' ? 20U : 0U;
        if (!amount) {
            show("Invalid payment", "5, 0, A or #");
            delay_ms(850);
        } else if (credit + amount > MAX_CREDIT) {
            show("Max credit Rs100", "Press D or #");
            delay_ms(1000);
        } else {
            credit += amount;
        }
    }
}

static unsigned char verify_pin(void)
{
    unsigned int i;
    char entry[5] = "____";
    char digits[5];
    char key;
    show("Admin PIN: 4 keys", entry);
    for (i = 0; i < 4;) {
        key = get_key();
        if (key == '#') return 0;
        if (key < '0' || key > '9') continue;
        digits[i] = key;
        entry[i++] = '*';
        lcd_line(1, entry);
    }
    for (i = 0; i < 4; ++i) {
        if (digits[i] != ADMIN_PIN[i]) {
            show("Incorrect PIN", "Access denied");
            delay_ms(1400);
            return 0;
        }
    }
    return 1;
}

static void admin_mode(void)
{
    char key, line[17];
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
                    products[item].stock);
            show(line, "*=Refill #=Back");
            key = get_key();
            if (key == '#') break;
            if (key == '*') {
                products[item].stock = INITIAL_STOCK;
                show("Restocked to 5", products[item].name);
                delay_ms(1000);
            }
        }
    }
}

int main(void)
{
    char key;
    unsigned char second_page = 0;
    SystemInit();
    SystemCoreClockUpdate();
    hardware_init();
    show("Vending machine", "Ready");
    delay_ms(1000);

    for (;;) {
        if (second_page) show("3 Snack  Rs15", "*=Menu A=Admin");
        else menu();
        key = get_key();
        if (key == '*') second_page = !second_page;
        else if (key >= '1' && key <= '3') purchase((unsigned int)(key-'1'));
        else if (key == 'A') admin_mode();
        else if (key == '#') {
            show("Select a product", "1, 2 or 3");
            delay_ms(900);
        } else {
            show("Invalid choice", "1-3 * or A");
            delay_ms(900);
        }
    }
}

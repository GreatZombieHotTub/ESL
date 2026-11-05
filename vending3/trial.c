#include <LPC17xx.h>
#include <stdint.h>
#include <stdio.h>
#include "LCD.h"

/* ALS-SDA-ARMCTXM3-01: CNB -> CNB3, CND -> CNAD, CNA -> CNA1. */
#define ROW_MASK (0xFUL << 10)  /* P2.10..13 */
#define COL_MASK (0xFUL << 23)  /* P1.23..26 */
#define LED_MASK (0xFDUL << 4)  /* P0.4 and P0.6..11 */
#define BZ_MASK  (1UL << 15)   /* Original source P0.15; verify BZ1 wiring */
#define INITIAL_STOCK 5U
#define MAX_ITEMS (4U * INITIAL_STOCK)
#define MAX_PRICE 500U
#define MAX_PAYMENT 9999U
#define ADMIN_PIN "1234"

typedef struct { const char *name; unsigned int price; unsigned char stock; } Product;
static Product products[4] = {
    {"Biscuits",12U,INITIAL_STOCK}, {"Water",10U,INITIAL_STOCK},
    {"Juice",20U,INITIAL_STOCK}, {"Snacks",15U,INITIAL_STOCK}
};
static unsigned char qty[4], order[MAX_ITEMS], count, pin_fails;
static unsigned int total;
static const char map[4][4] = {
    {'1','2','3','A'}, {'4','5','6','B'},
    {'7','8','9','C'}, {'*','0','#','D'}
};

/* No RTOS: the original polled SysTick delay remains valid. */
static void delay_us(unsigned int us) {
    unsigned long ticks=SystemCoreClock/1000000UL;
    if (!us || !ticks) return;
    SysTick->LOAD=ticks*us-1UL; SysTick->VAL=0; SysTick->CTRL=5UL;
    while (!(SysTick->CTRL & (1UL<<16))) { }
    SysTick->CTRL=0;
}
void delay_ms(unsigned int ms) { while (ms--) delay_us(1000U); }
static void line(unsigned int r,const char *s) {
    char b[17]; unsigned int i=0;
    while (i<16 && s[i]) { b[i]=s[i]; i++; }
    while (i<16) b[i++]=' ';
    b[16]=0; lcd_line(r,b);
}
static void show(const char *a,const char *b) { line(0,a); line(1,b); }

/* UART0 TX is P0.2, exposed through the board's RS232 interface CN9. */
static void serial_init(void) {
    unsigned int divisor;
    LPC_SC->PCONP |= 1UL<<3;
    LPC_SC->PCLKSEL0 &= ~(3UL<<6); /* UART0 PCLK = CCLK/4 */
    LPC_PINCON->PINSEL0 = (LPC_PINCON->PINSEL0 & ~(3UL<<4)) | (1UL<<4);
    divisor=(SystemCoreClock/4U + 76800U)/(16U*9600U);
    LPC_UART0->LCR=0x83; LPC_UART0->DLM=(divisor>>8)&255U;
    LPC_UART0->DLL=divisor&255U; LPC_UART0->FDR=0x10;
    LPC_UART0->LCR=0x03; LPC_UART0->FCR=0x07;
}
static void send(const char *s) {
    while (*s) { while (!(LPC_UART0->LSR & (1UL<<5))) { } LPC_UART0->THR=(unsigned char)*s++; }
    while (!(LPC_UART0->LSR & (1UL<<5))) { } LPC_UART0->THR='\n';
}
static void event4(const char *name,unsigned int a,unsigned int b,unsigned int c,unsigned int d) {
    char msg[72]; snprintf(msg,sizeof msg,"%s,%u,%u,%u,%u",name,a,b,c,d); send(msg);
}
static void beep(unsigned int hz,unsigned int ms) {
    unsigned int half=500000U/hz, cycles=(1000U*ms)/(2U*half),i;
    for (i=0;i<cycles;i++) {
        LPC_GPIO0->FIOSET=BZ_MASK; delay_us(half);
        LPC_GPIO0->FIOCLR=BZ_MASK; delay_us(half);
    }
}
static char scan(void) {
    unsigned int r,c,bits;
    LPC_GPIO2->FIOCLR=ROW_MASK;
    for (r=0;r<4;r++) {
        LPC_GPIO2->FIOSET=1UL<<(10+r); delay_ms(1);
        bits=(LPC_GPIO1->FIOPIN & COL_MASK)>>23;
        LPC_GPIO2->FIOCLR=1UL<<(10+r);
        for (c=0;c<4;c++) if (bits & (1UL<<c)) return map[r][c];
    }
    return 0;
}
static char key(void) {
    char k;
    for (;;) {
        do { k=scan(); } while (!k);
        delay_ms(25);
        if (scan()==k) break;
    }
    while (scan()) delay_ms(10);
    delay_ms(25); return k;
}
static void init(void) {
    unsigned int i;
    for (i=0;i<4;i++) {
        LPC_PINCON->PINSEL3 &= ~(3UL<<(14+2*i));
        LPC_PINCON->PINSEL4 &= ~(3UL<<(20+2*i));
        LPC_PINCON->PINMODE3 = (LPC_PINCON->PINMODE3 & ~(3UL<<(14+2*i))) | (3UL<<(14+2*i));
    }
    for (i=23;i<=28;i++) LPC_PINCON->PINSEL1 &= ~(3UL<<(2*(i-16)));
    LPC_PINCON->PINSEL0 &= ~0x00FFFF00UL;
    LPC_PINCON->PINSEL0 &= ~(3UL<<30);
    LPC_GPIO2->FIODIR |= ROW_MASK; LPC_GPIO2->FIOCLR=ROW_MASK;
    LPC_GPIO1->FIODIR &= ~COL_MASK;
    LPC_GPIO0->FIODIR |= LED_MASK|BZ_MASK;
    LPC_GPIO0->FIOCLR=LED_MASK|BZ_MASK;
    lcd_init(); serial_init(); send("READY,1");
}
static void clear_cart(void) {
    unsigned int i; count=0; total=0;
    for (i=0;i<4;i++) qty[i]=0;
    event4("CART",0,0,0,0);
}
static void add(unsigned int p) {
    char b[17];
    if (count>=MAX_ITEMS || qty[p]>=products[p].stock) {
        show("Stock/cart limit",products[p].name); send("ERROR,STOCK");
        beep(220,150); delay_ms(850); return;
    }
    order[count++]=(unsigned char)p; qty[p]++; total+=products[p].price;
    snprintf(b,sizeof b,"Qty%u Total%u",(unsigned)qty[p],total);
    show(products[p].name,b);
    event4("SELECT",p+1U,qty[p],total,products[p].stock);
    delay_ms(650);
}
static void undo(void) {
    unsigned int p;
    if (!count) { show("Cart empty","Select 1-4"); delay_ms(600); return; }
    p=order[--count]; qty[p]--; total-=products[p].price;
    event4("REMOVE",p+1U,qty[p],total,0); show("Removed last",products[p].name); delay_ms(650);
}
/* Keypad 0-9, D to confirm, * to erase last digit, # to cancel. */
static int number(const char *title,unsigned int limit,unsigned int *out) {
    unsigned int n=0,digits=0,d; char k,b[17];
    show(title,"0-9 D=OK #=X");
    for (;;) {
        k=key();
        if (k=='#') return 0;
        if (k=='*') { if (digits) { n/=10U; digits--; } }
        else if (k=='D') {
            if (digits) { *out=n; return 1; }
            show("Empty amount","Type 0-9 first"); beep(220,120); delay_ms(650);
        } else if (k>='0' && k<='9') {
            d=(unsigned int)(k-'0');
            if (n>(limit-d)/10U) {
                show("Amount too large","*=Erase #=Exit"); beep(220,120); delay_ms(650);
            } else { n=n*10U+d; digits++; }
        }
        snprintf(b,sizeof b,"Rs%u D=OK *=Del",n); show(title,b);
    }
}
static void dispense(unsigned int p,unsigned int n,unsigned int n_total) {
    static const unsigned char pins[7]={4,6,7,8,9,10,11};
    unsigned int i; char b[17];
    event4("DISPENSE",p+1U,n,n_total,0);
    snprintf(b,sizeof b,"Dispense %u/%u",n,n_total); show(b,products[p].name);
    LPC_GPIO0->FIOCLR=LED_MASK;
    for (i=0;i<7;i++) { LPC_GPIO0->FIOSET=1UL<<pins[i]; delay_ms(140); }
    LPC_GPIO0->FIOCLR=LED_MASK;
}
static void checkout(void) {
    unsigned int paid,change,i,n; char b[17];
    if (!count) { show("Cart empty","Select 1-4"); send("ERROR,EMPTY"); delay_ms(850); return; }
    snprintf(b,sizeof b,"Total Rs%u",total);
    if (!number(b,MAX_PAYMENT,&paid)) { clear_cart(); send("CANCEL,0"); show("Cancelled","Cart cleared"); delay_ms(700); return; }
    if (paid<total) {
        snprintf(b,sizeof b,"Need Rs%u",total-paid); show("Not enough",b);
        event4("PAYMENT_FAIL",paid,total,0,0); beep(220,200); delay_ms(1150); return;
    }
    change=paid-total; n=count;
    for (i=0;i<4;i++) products[i].stock-=qty[i];
    event4("PAYMENT_OK",paid,total,change,0);
    for (i=0;i<n;i++) dispense(order[i],i+1U,n);
    clear_cart(); event4("DONE",paid,change,0,0);
    beep(660,120); beep(880,140);
    snprintf(b,sizeof b,"Change Rs%u",change); show("Purchase done",b); delay_ms(1600);
}
static int verify_pin(void) {
    unsigned int i,j; char entered[4],stars[5]="____",k;
    show("Admin PIN",stars);
    for (i=0;i<4;) {
        k=key(); if (k=='#') return 0;
        if (k>='0' && k<='9') { entered[i]=k; stars[i++]='*'; line(1,stars); }
    }
    for (j=0;j<4;j++) if (entered[j]!=ADMIN_PIN[j]) break;
    if (j!=4) {
        show("Wrong PIN","Access denied"); beep(220,250); delay_ms(900);
        if (++pin_fails>=3) { pin_fails=0; show("Too many tries","Wait 10 sec"); delay_ms(10000); }
        return 0;
    }
    pin_fails=0; return 1;
}
static void admin(void) {
    unsigned int p,v; char k,b[17];
    if (!verify_pin()) return;
    for (;;) {
        show("Admin: 1-4", "#=Exit"); k=key();
        if (k=='#') return;
        if (k<'1' || k>'4') continue;
        p=(unsigned int)(k-'1');
        for (;;) {
            snprintf(b,sizeof b,"Rs%u Stock%u",products[p].price,(unsigned)products[p].stock);
            show(products[p].name,b); delay_ms(800);
            show("A=Price *=Fill", "#=Back"); k=key();
            if (k=='#') break;
            if (k=='*') {
                products[p].stock=INITIAL_STOCK; event4("STOCK",p+1U,INITIAL_STOCK,0,0);
                show("Stock refilled",products[p].name); delay_ms(800);
            }
            if (k=='A' && number("New price Rs",MAX_PRICE,&v)) {
                if (v==0) { show("Price must be >0","No change"); delay_ms(800); }
                else { products[p].price=v; event4("PRICE",p+1U,v,0,0);
                       show("Price changed",products[p].name); delay_ms(800); }
            }
        }
    }
}
int main(void) {
    char k,b[17]; unsigned int i;
    SystemInit(); SystemCoreClockUpdate(); init();
    show("Vending machine","1-4 choose D pay"); delay_ms(1200);
    for (;;) {
        snprintf(b,sizeof b,"Items%u Rs%u",(unsigned)count,total);
        show("1B 2W 3J 4S",b); k=key();
        if (k>='1' && k<='4') add((unsigned int)(k-'1'));
        else if (k=='D') checkout();
        else if (k=='*') undo();
        else if (k=='#') { clear_cart(); send("CANCEL,0"); show("Cart cancelled","Select 1-4"); delay_ms(700); }
        else if (k=='C') {
            snprintf(b,sizeof b,"B%u W%u J%u S%u",(unsigned)qty[0],(unsigned)qty[1],
                     (unsigned)qty[2],(unsigned)qty[3]);
            show(b,"D=Pay *=Undo"); delay_ms(1200);
        } else if (k=='B' && !count) admin();
        else if (k=='A') {
            for (i=0;i<4;i++) event4("PRODUCT",i+1U,products[i].price,products[i].stock,0);
            show("Prices sent","PC display"); delay_ms(700);
        }
    }
}

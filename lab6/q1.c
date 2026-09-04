#include <LPC17xx.h>

// Aggressive tempo for a fast, club-style strobe effect
void delay_beat(unsigned int tempo)
{
    unsigned int i;
    for(i = 0; i < tempo; i++);
}

int main(void)
{
    int i;
    SystemInit();
    SystemCoreClockUpdate();

    // Configure Port0 pins P0.4 to P0.11 as GPIO function
    LPC_PINCON->PINSEL0 &= 0xFF0000FF; 
    
    // Set P0.4 to P0.11 as output lines
    LPC_GPIO0->FIODIR |= 0x00000FF0; 

    while(1)
    {
        // Disco Pattern 1: Rapid Alternating Strobe (Odds vs Evens)
        for(i = 0; i < 15; i++)
        {
            LPC_GPIO0->FIOSET = 0x00000AA0; // Turn ON P0.5, P0.7, P0.9, P0.11
            LPC_GPIO0->FIOCLR = 0x00000550; // Ensure others are OFF
            delay_beat(3000000); 
            
            LPC_GPIO0->FIOCLR = 0x00000AA0; // Turn OFF odds
            LPC_GPIO0->FIOSET = 0x00000550; // Turn ON P0.4, P0.6, P0.8, P0.10
            delay_beat(3000000);
        }
        LPC_GPIO0->FIOCLR = 0x00000FF0; // Clear all pins

        // Disco Pattern 2: The "Split & Merge" (Outside-In Collision)
        for(i = 0; i < 5; i++)
        {
            // Outer Edges (P0.4, P0.11)
            LPC_GPIO0->FIOSET = 0x00000810; 
            delay_beat(4000000);
            LPC_GPIO0->FIOCLR = 0x00000810;
            
            // Inner Edges (P0.5, P0.10)
            LPC_GPIO0->FIOSET = 0x00000420;
            delay_beat(4000000);
            LPC_GPIO0->FIOCLR = 0x00000420;

            // Mid-Center (P0.6, P0.9)
            LPC_GPIO0->FIOSET = 0x00000240;
            delay_beat(4000000);
            LPC_GPIO0->FIOCLR = 0x00000240;

            // Center Smash (P0.7, P0.8)
            LPC_GPIO0->FIOSET = 0x00000180;
            delay_beat(4000000);
            LPC_GPIO0->FIOCLR = 0x00000180;
        }

        // Disco Pattern 3: The "Flashbang" (Hyper-Strobe)
        for(i = 0; i < 20; i++)
        {
            LPC_GPIO0->FIOSET = 0x00000FF0; // Full blast
            delay_beat(1000000);            // Extreme short delay
            LPC_GPIO0->FIOCLR = 0x00000FF0; // Total blackout
            delay_beat(1000000);
        }
    }
}
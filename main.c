#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "lcd.h"
#include "../Common/Include/stm32l051xx.h"

#define F_CPU 32000000L

void Delay_us(unsigned char us)
{
	// For SysTick info check the STM32L0xxx Cortex-M0 programming manual page 85.
	SysTick->LOAD = (F_CPU/(1000000L/us)) - 1;  // set reload register, counter rolls over from zero, hence -1
	SysTick->VAL = 0; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while((SysTick->CTRL & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SysTick->CTRL = 0x00; // Disable Systick counter
}

void delay(int dly)
{
	while( dly--);
}

void wait_1ms(void)
{
	// For SysTick info check the STM32L0xxx Cortex-M0 programming manual page 85.
	SysTick->LOAD = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	SysTick->VAL = 0; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while((SysTick->CTRL & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SysTick->CTRL = 0x00; // Disable Systick counter
}

void waitms(int len)
{
	while(len--) wait_1ms();
}

void LCD_pulse (void)
{
	LCD_E_1;
	Delay_us(40);
	LCD_E_0;
}

void LCD_byte (unsigned char x)
{
	//Send high nible
	if(x&0x80) LCD_D7_1; else LCD_D7_0;
	if(x&0x40) LCD_D6_1; else LCD_D6_0;
	if(x&0x20) LCD_D5_1; else LCD_D5_0;
	if(x&0x10) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
	Delay_us(40);
	//Send low nible
	if(x&0x08) LCD_D7_1; else LCD_D7_0;
	if(x&0x04) LCD_D6_1; else LCD_D6_0;
	if(x&0x02) LCD_D5_1; else LCD_D5_0;
	if(x&0x01) LCD_D4_1; else LCD_D4_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS_1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS_0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E_0; // Resting state of LCD's enable is zero
	//LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, unsigned char column, unsigned char clear)
{
	int j;

	WriteCommand(line==2?0xc0+(column-1):0x80+(column-1));
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

#define START_BUTTON (GPIOA->IDR&BIT6)
#define PIN_PERIOD (GPIOA->IDR&BIT8)


// GetPeriod() seems to work fine for frequencies between 300Hz and 600kHz.
// 'n' is used to measure the time of 'n' periods; this increases accuracy.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	unsigned char overflow;
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	overflow=0;
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(SysTick->CTRL & BIT16) overflow++;
		if (overflow>10) return 0;

	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	SysTick->LOAD = 0xffffff;  // 24-bit counter set to check for signal present
	SysTick->VAL = 0xffffff; // load the SysTick counter
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(SysTick->CTRL & BIT16) overflow++;
		if (overflow>10) return 0;
	}
	SysTick->CTRL = 0x00; // Disable Systick counter
	
	SysTick->LOAD = 0xffffff;  // 24-bit counter reset
	SysTick->VAL = 0xffffff; // load the SysTick counter to initial value
	SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk; // Enable SysTick IRQ and SysTick Timer */
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(SysTick->CTRL & BIT16) overflow++;
			if (overflow>10) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(SysTick->CTRL & BIT16) overflow++;
			if (overflow>10) return 0;
		}
	}
	SysTick->CTRL = 0x00; // Disable Systick counter

	return (overflow*0x01000000)+(0xffffff-SysTick->VAL);
}

void debounce_signal(unsigned int delay_ms, unsigned char pin){
    unsigned char i = 0;
    char current_state = pin; // Read the initial state of the pin
    while(i < delay_ms){
        if(pin == current_state){
            // If the pin state matches the expected state, proceed
            waitms(1); // Wait for 1 ms
            i++; // Only increment if the state remains unchanged
        } else {
            // If the pin state has changed, reset the counter and update the expected state
            i = 0;
            current_state = pin;
        }
    }
}

void Configure_Pins (void)
{
	RCC->IOPENR |= (BIT0 | BIT1); // Enable clock for GPIOA (BIT0) and GPIOB (BIT1)

	// Make pins PA0 to PA5 outputs (page 200 of RM0451, two bits used to configure: bit0=1, bit1=0)
    GPIOA->MODER = (GPIOA->MODER & ~(BIT0|BIT1)) | BIT0; // PA0
	GPIOA->OTYPER &= ~BIT0; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT2|BIT3)) | BIT2; // PA1
	GPIOA->OTYPER &= ~BIT1; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT4|BIT5)) | BIT4; // PA2
	GPIOA->OTYPER &= ~BIT2; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT6|BIT7)) | BIT6; // PA3
	GPIOA->OTYPER &= ~BIT3; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT8|BIT9)) | BIT8; // PA4
	GPIOA->OTYPER &= ~BIT4; // Push-pull
    
    GPIOA->MODER = (GPIOA->MODER & ~(BIT10|BIT11)) | BIT10; // PA5
	GPIOA->OTYPER &= ~BIT5; // Push-pull
	
	GPIOA->MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	// Activate pull up for pin PA8:
	GPIOA->PUPDR |= BIT16; 
	GPIOA->PUPDR &= ~(BIT17);
	
	GPIOA->MODER &= ~(BIT12 | BIT13); // Make pin PA6 input
	// Activate pull up for pin PA6:
	GPIOA->PUPDR |= BIT12; 
	GPIOA->PUPDR &= ~(BIT13);
	
}

// LQFP32 pinout
//             ----------
//       VDD -|1       32|- VSS
//      PC14 -|2       31|- BOOT0
//      PC15 -|3       30|- PB7
//      NRST -|4       29|- PB6
//      VDDA -|5       28|- PB5
//       PA0 -|6       27|- PB4
//       PA1 -|7       26|- PB3
//       PA2 -|8       25|- PA15
//       PA3 -|9       24|- PA14
//       PA4 -|10      23|- PA13
//       PA5 -|11      22|- PA12
//       PA6 -|12      21|- PA11
//       PA7 -|13      20|- PA10 (Reserved for RXD)
//       PB0 -|14      19|- PA9  (Reserved for TXD)
//       PB1 -|15      18|- PA8  (Measure the period at this pin)
//       VSS -|16      17|- VDD
//             ----------

void main(void)
{
	long int count;
	float T;
	unsigned int bpm;
	char bpm_lcd[4];
	char second_row[17];
	char unit[5] = {' ','B','P','M','\0'};
	int i;
	
	Configure_Pins();
	LCD_4BIT();
	
	LCDprint("Heart Rate Mon.", 1, 1, 1);
	LCDprint("Start", 2, 1, 1);
	
	while(START_BUTTON != 0);
	debounce_signal(25, START_BUTTON);
	while(START_BUTTON != 0);
	if(START_BUTTON != 1){}
	debounce_signal(25, START_BUTTON);
	if(START_BUTTON != 1)
	{
	
		LCDprint("Please put finger", 1, 1, 1);
		LCDprint("in the sensor.", 2, 1, 1);
		
		while(PIN_PERIOD != 0); 
		
		waitms(2000);
		
		LCDprint("Finger Detected.", 1, 1, 1);
		LCDprint("                ", 2, 1, 1);
		
		waitms(2000);
		
		LCDprint("Heart Rate:", 1, 1, 1);   
		LCDprint("                ", 2, 1, 1);
		waitms(500); // Give PuTTY a chance to start.
		printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
		
		while(START_BUTTON != 0)
		{
			count=GetPeriod(1);
			if(count>0)
			{
				T=(count*1.0)/(F_CPU); // Since we have the time of 100 periods, we need to divide by 100
				bpm=60.0/T + 0.5;
				if(50 < bpm && bpm < 110)
				{
					GPIOA->MODER = (GPIOA->MODER & ~(BIT15|BIT14)) | BIT14;
					GPIOA->OTYPER &= ~BIT7; // PA7 as push-pull
			
					GPIOA->ODR &= ~BIT7; // Toggle PA7
					waitms(500);
					GPIOA->ODR |= BIT7;
					
					
					sprintf(bpm_lcd, "%u", bpm);
					for(i = 0; i < strlen(bpm_lcd); i++){
					   	second_row[i] = bpm_lcd[i];
					
					}
					
					for(i = 0; i < strlen(unit); i++){
					   	second_row[i+strlen(bpm_lcd)] = unit[i];
					
					}
					
					for(i = 0; i < strlen(second_row)-strlen(bpm_lcd)-strlen(unit); i++){
					   	second_row[i+strlen(bpm_lcd)+strlen(unit)] = '\0';
					
					}
					
					LCDprint(second_row, 2, 1, 1);
					printf("\r%u\n", bpm);
				}
				
				else if(bpm > 110 && bpm < 220)
				{
					GPIOB->MODER = (GPIOB->MODER & ~(BIT1|BIT0)) | BIT0;
					GPIOB->OTYPER &= ~BIT0; // PB0 as push-pull
					GPIOB->ODR &= ~BIT0;
					waitms(500);
					GPIOB->ODR |= BIT0; // Toggle PB0
					sprintf(bpm_lcd, "%u", bpm);
					for(i = 0; i < strlen(bpm_lcd); i++){
					   	second_row[i] = bpm_lcd[i];
					
					}
					
					for(i = 0; i < strlen(unit); i++){
					   	second_row[i+strlen(bpm_lcd)] = unit[i];
					
					}
					
					for(i = 0; i < strlen(second_row)-strlen(bpm_lcd)-strlen(unit); i++){
					   	second_row[i+strlen(bpm_lcd)+strlen(unit)] = '\0';
					
					}
					
					LCDprint(second_row, 2, 1, 1);
					printf("\r%u\n", bpm);
				}
				
				else if (bpm > 220)
				{
					LCDprint("NO SIGNAL          ", 2, 1, 1);
				}
				
			}
			else
			{
				//printf("NO SIGNAL                     \r");
				LCDprint("NO SIGNAL          ", 2, 1, 1);
			}
			fflush(stdout); // GCC printf wants a \n in order to send something.  If \n is not present, we fflush(stdout)
			waitms(200);
			
		}
		
		GPIOA->ODR |= BIT7;
		GPIOB->ODR |= BIT0;
		LCDprint("Done.", 1, 1, 1);
		LCDprint("         ", 2, 1, 1);

		waitms(5000);

		main();
	}
	
}

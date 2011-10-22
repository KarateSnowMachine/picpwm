#include <p18f4550.h>



#pragma config PBADEN=OFF
typedef unsigned char uchar; 
void delay_little()
{
int i=0;
for (i=0; i<1; i++)
{;}

}
void delay()
{
int i=0;
for (i=0; i<20; i++)
{;}

}

void init_spi()
{

	// set tristate bits
	TRISCbits.TRISC7 = 0; // output on SDO

	TRISAbits.TRISA5 = 1; // SS`  

	TRISBbits.TRISB0 = 1; // input for SDI
	TRISBbits.TRISB1 = 0; // output on SCK
	TRISBbits.TRISB2 = 0; // OE` on RB0 = output
	TRISBbits.TRISB3 = 0; // OE` on RB0 = output
	TRISBbits.TRISB4 = 0; // OE` on RB0 = output
	TRISBbits.TRISB5 = 0; // OE` on RB0 = output
	
	
	
	SSPCON1bits.SSPEN = 1; // enable MSSP

	// Setup other stuff
	SSPCON1 = 0; // Fosc/4 frequency, CKP=0
	ADCON1 = 0x0f;
	SSPCON1bits.CKP=1;
	SSPCON1bits.SSPEN = 1; 
	SSPSTATbits.CKE = 0;
}

uchar _rotl(uchar value) {
    return (value << 1) | (value >> 7);
}

#define NUM_LEDS 5

#define LED_RED 0
#define LED_GREEN 1
#define LED_BLUE 2
#define LED_NUM_TYPES 3

uchar led_status[LED_NUM_TYPES] = {0, 0, 0}; 

void set_on(uchar type, uchar number)
{
	led_status[type] |= (1<<number);
}
void set_off(uchar type, uchar number)
{
	led_status[type] &= ~(1<<number);
}

#define BREG(n) LATBbits.LATB##n


void draw()
{
	uchar i=0;
	//turn off register output
	BREG(5) = 1;
//	BREG(4) = 1;
//	BREG(3) = 1;
	

	SSPBUF = ((~led_status[LED_BLUE]) << (1+i)) | ((1<<(i+1))-1);
	//--i;

	delay_little(); 

	SSPBUF = ((~led_status[LED_GREEN]) << (1+i)) | ((1<<(i+1))-1);
	//--i;
	delay_little(); 

	SSPBUF = ((~led_status[LED_RED]) << (1+i)) | ((1<<(i+1))-1);
	//turn on register output
	BREG(5) = 0;
//	BREG(4) = 0;
//	BREG(3) = 0;
	delay(); 
	
}
#define NUM_ITERS 10
	
void duty(uchar duty_cycle, uchar type, uchar num) 
{
	uchar cycles; 
	uchar other_type =(type+1)%LED_NUM_TYPES;
	uchar other_num = (num+1)%NUM_LEDS;
	//uchar other_type2 = LED_BLUE;
	//uchar other_num2 = (num+2)%NUM_LEDS;
	set_off(type,num);
	//set_off(other_type, other_num);
	//set_off(other_type2, other_num2);  
	for (cycles=0; cycles<NUM_ITERS; cycles++)
	{
		if (cycles == NUM_ITERS-duty_cycle) 
		{	
			set_on(type, num);
			//set_on(other_type2, other_num2);  
		}
		draw(); 
	}
}
void main ()
{
	uchar duty_cycle,x; 
	uchar cycles;
	uchar i,j;
	init_spi(); 


	for (i=0; i<5; i++) 
	{
		for (duty_cycle=0; duty_cycle<NUM_ITERS; duty_cycle++)
		{
			duty(duty_cycle, LED_RED, i );
		}
	}
	for (i=0; i<5; i++) 
	{
		for (duty_cycle=0; duty_cycle<NUM_ITERS; duty_cycle++)
		{
			duty(duty_cycle, LED_BLUE, NUM_LEDS-i-1);
		}
	}
	for (i=0; i<5; i++) 
	{
		for (duty_cycle=0; duty_cycle<NUM_ITERS; duty_cycle++)
		{
			duty(duty_cycle, LED_GREEN, i);
		}
	}

while(1) {};

}

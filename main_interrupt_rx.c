#include <p18f4550.h>




#pragma config PBADEN=OFF
void interrupt_draw(void);
void draw(void); 
#pragma code high_vector=0x08
void interrupt_draw()
{
	INTCONbits.TMR0IE = 0; 
	draw();
	INTCONbits.TMR0IF = 0;
	INTCONbits.TMR0IE = 1;
	return;
}


#pragma code
#pragma interrupt interrupt_draw

typedef unsigned char uchar; 
extern void spi_write(uchar byte);


#define NUM_ROWS 8
#define NUM_COLS 4
#define NUM_LEDS (NUM_ROWS*NUM_COLS)

#define LED_RED 0
#define LED_GREEN 1
#define LED_BLUE 2
#define LED_NUM_TYPES 3

void config_timer0()
{
	#if (NUM_ROWS == 8)
	//	bits: 76543210
	T0CON = 0b11011000; // 1:16
	#elif (NUM_ROWS==4)
	//	bits: 76543210
	T0CON = 0b11010010; // 1:8
	#else
	T0CON = 0b11010010; // 1:8
	#endif
	// summary: enable timer0, 8 bit counter, CLKO internal clock, prescale 1:32

	RCONbits.IPEN=0;  //enable interrupt priority
	INTCONbits.TMR0IF = 0; //clear flag
	INTCONbits.TMR0IE = 1; // enable timer interrupt
	INTCON2bits.TMR0IP = 1; // timer0 = high priority
	INTCONbits.GIE = 1;  	//enable high priority interrupts

// for Timer0 source = Fosc/4 = 2MHz and a target timer of 480Hz, we need: 
//		2MHz/240 = 1/4.166k scaler 
//		With an 8bit timer, the implicit scaler is 1/256, which leaves roughly a 1/32 ratio left for the prescalar
// 		This means: 2MHz/(256*16) ~= 488Hz, which is good enough here



	//and off we go!
}
void init_io_pins()
{
	// set tristate bits
	TRISCbits.TRISC7 = 0; // output on SDO
	TRISAbits.TRISA5 = 1; // SS`  

	TRISBbits.TRISB0 = 1; // input for SDI
	TRISBbits.TRISB1 = 0; // output on SCK
	TRISBbits.TRISB2 = 0; // RB2 out -- OE`
	TRISBbits.TRISB3 = 0; // RB3 out
	TRISBbits.TRISB4 = 0; // RB4 out
	TRISBbits.TRISB5 = 0; // RB5 out
	
	// Setup other stuff
	SSPCON1 = 0; // Fosc/4 frequency, CKP=0
	ADCON1 = 0x0f;
	
	SSPCON1bits.SSPEN = 1; // enable MSSP
	SSPCON1bits.CKP=1;
	SSPSTATbits.CKE = 0;
}
	

#define NUM_INTENSITY_LEVELS 8
uchar all_cycles = 0;
uchar pwm_cycle = 0;
uchar refresh_row =0 ; 
uchar led_status[LED_NUM_TYPES][NUM_ROWS] = {{0}}; 

void set_on(uchar type, uchar number)
{
	uchar pos;

 	pos = number%NUM_COLS;
	led_status[type][number/NUM_COLS] |= (1<<pos);
}
void set_off(uchar type, uchar number)
{
	uchar pos;
	pos = number%NUM_COLS;
	led_status[type][number/NUM_COLS] &= ~(1<<pos);
}

#define BREG(n) LATBbits.LATB##n


void refresh_pwm(uchar type, uchar num, uchar duty_cycle)
{
	if (pwm_cycle >= (NUM_INTENSITY_LEVELS - duty_cycle))
	{
		set_on(type, num); 
	}
	else
	{
		set_off(type, num); 
	}
}
uchar num=0; 
uchar current_led=0;

void draw()
{

	uchar row_pattern = ~(1<<(refresh_row+1));
	//TODO: add inverter to make the <<1 unnecessary 
	uchar r_pattern = ~((led_status[LED_RED][refresh_row]) <<1);
	uchar g_pattern = ~((led_status[LED_GREEN][refresh_row]) <<1);
	uchar b_pattern = ~((led_status[LED_BLUE][refresh_row]) << 1);

	//turn off register output
	BREG(2) = 1; 

	spi_write(row_pattern);
	spi_write(b_pattern); 
	spi_write(g_pattern); 
	spi_write(r_pattern); 
	// re-enable outputs

	BREG(2) = 0; 

	refresh_row = (refresh_row + 1) % NUM_ROWS;
	if (refresh_row == 0)
	{
		#define NUM 32 // sizeof(uchar) in bits / # intensity levels
		pwm_cycle = (pwm_cycle +1) % NUM_INTENSITY_LEVELS; 		
		if (pwm_cycle==0)
			current_led = (current_led+1) % 8;
		refresh_pwm(LED_GREEN,current_led, num/NUM); 
		refresh_pwm(LED_RED,current_led, NUM_INTENSITY_LEVELS-num/NUM); 

		//refresh_pwm(LED_BLUE,2, NUM_INTENSITY_LEVELS-num/NUM);

		num++;
	}
}

void main ()
{			
			 
	
	uchar duty_cycle,x; 
	uchar cycles;
	uchar i,j=5;
	init_io_pins(); 
		      //76543210
	OSCCON = 0b11111111;
	// TODO: check to make sure that timer is stable before going on 

	config_timer0();
//	set_on(LED_RED,0); 
//	set_on(LED_BLUE, 4); 
	while(1); 
	/*
	
	while(1)
	{
		for (i=0; i< NUM_INTENSITY_LEVELS; i++)
		{
			if (pwm_cycle == 0)
			{
				set_on(LED_RED,0); 
			}
			else
			{
				set_off(LED_RED,0); 
			}
		}
		i = (i+1) % NUM_INTENSITY_LEVELS;
	}
*/
}

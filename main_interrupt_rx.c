#include <pic18f4550.h>

#pragma config XINST=OFF
#pragma config PBADEN=OFF
#pragma config FOSC=HSPLL_HS
#pragma config PLLDIV=5
#pragma config CPUDIV=OSC1_PLL2

#include "led.h"


void draw(void); 

//#pragma code high_vector=0x08


void init_oscillator()
{
	// internal oscillator 
	//OSCCON = 0b11111111; 
	// external (primary) oscillator
	OSCCON = 0b11111100; 
}
void init_timer0()
{
	T0CON = 0b11011000; // no prescalar

	//	bits: 76543210
//	T0CON = 0b11010000; // 1:2 (0_000)
	

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
//#pragma code
//#pragma interrupt interrupt_draw

typedef unsigned char uchar; 
//#define PIR1 0x0F9E
//#define PIR2 0x0FA1
//#define PLUSW1 0x0FE3
//#define SSPIF 0x0003
//#define SSPBUF 0x0FC9

void spi_write(uchar byte_to_write) {
    if (!PIR1bits.SSPIF) {
        SSPBUF = byte_to_write;
         
        while(PIR1bits.SSPIF) {
            //spin
        }
    }
}
#if 0
void spi_write(uchar byte_to_write) {
uchar x = byte_to_write;
__asm

	;read in the argument (byte to write) -- see C18 User's Guide page 44
	movlw 	0xff
	movf	PLUSW1, W, A ; WREG = *(FSR1 - 1)
	;now W = byte_to_write

; This code taken from pic18f4550 datasheet page 200 
; TransmitSPI:
		BCF PIR2, SSPIF ;Make sure interrupt flag is clear 
		MOVWF SSPBUF, A ;Load data to send into transmit buffer
;Loop until data has finished transmitting
    foo: 
		BTFSS PIR1, SSPIF ;Interrupt flag set when transmit is complete
	bra foo
__endasm ;
}
#endif

	
typedef struct led_state
{
	uchar R;
	uchar G;
	uchar B;
}led_state_t;

typedef struct led_fade
{
	unsigned int prescaler;
	unsigned int prescaler_counter;
	uchar steps;
	char dR; 
	char dG;
	char dB;
} led_fade_t;

uchar all_cycles = 0;
uchar pwm_cycle = 0;
uchar refresh_row =0 ; 

// This assumes 8 LEDs per column = 8 bits in a uchar, i.e. the bits of each char are the column patterns for a given row in the matrix
uchar led_status[LED_NUM_TYPES][NUM_ROWS];// = {{0}}; 

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

led_state_t led_colors[NUM_LEDS];// = {0};
led_state_t led_target_states[NUM_LEDS];// = {0};

void refresh_pwm(uchar refresh_row)
{

	uchar i,refresh_led;
	led_state_t *led;
	
	for (i=0; i<NUM_COLS; i++)
	{
		refresh_led = refresh_row*NUM_COLS+i;
 		led = &led_colors[refresh_led];
		// TODO: This is kludgey :| 

		// Red:	
		if (pwm_cycle >= (NUM_INTENSITY_LEVELS - led->R)) {
			set_on(LED_RED, refresh_led); 
		}
		else {
			set_off(LED_RED, refresh_led); 
		}
		// Green:	
		if (pwm_cycle >= (NUM_INTENSITY_LEVELS - led->G)) {
			set_on(LED_GREEN, refresh_led); 
		}
		else {
			set_off(LED_GREEN, refresh_led); 
		}
		// Blue:	
		if (pwm_cycle >= (NUM_INTENSITY_LEVELS - led->B)) {
			set_on(LED_BLUE, refresh_led); 
		}
		else {
			set_off(LED_BLUE, refresh_led); 
		}
	}
}
led_fade_t fades[NUM_LEDS];// = {0}; 
void update_fade(uchar led)
{
	led_fade_t *f = &fades[led];
	if (f->steps == 0)
	{
		return;
	}
	if (f->prescaler_counter++ == f->prescaler)	{
		led_colors[led].R += f->dR; 
		led_colors[led].G += f->dG; 
		led_colors[led].B += f->dB; 
		f->prescaler_counter = 0;
		f->steps--; 
	} else {
		f->prescaler_counter++;
	}
	return;
}



void update_all_fades() 
{
	uchar led;
	for (led=0; led<NUM_LEDS; led++)
	{
		if (fades[led].steps > 0)
		{
			update_fade(led); 
		}
	}

}
void draw()
{
	uchar row_pattern = ~(1<<(refresh_row+1));
	//TODO: add inverter to make the <<1 unnecessary (also to make all 8 bits usable per register)
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
	refresh_pwm(refresh_row); 
	refresh_row = (refresh_row + 1) % NUM_ROWS;
	if (refresh_row == 0)
	{
		pwm_cycle = (pwm_cycle +1) % NUM_INTENSITY_LEVELS;
		update_all_fades();
	}
}

void set_rgb(uchar num, uchar r, uchar g, uchar b)
{
	led_colors[num].R = r; 
	led_colors[num].G = g;
	led_colors[num].B = b;
}

void set_fade(uchar num, uchar dR, uchar dG, uchar dB, uchar steps, unsigned int prescaler)
{
	led_fade_t *f;
	if (steps ==0)
	{
		return;
	}
	f = &fades[num];
	f->dR = dR;
	f->dG = dG;
	f->dB = dB;
	f->steps = steps;
	f->prescaler = prescaler;
	f->prescaler_counter = 0;
}

uchar is_fading(uchar num)
{
	return (fades[num].steps > 0);
}

void set_fade_blocking(uchar num, uchar dR, uchar dG, uchar dB, uchar steps, unsigned int prescaler)
{
	set_fade(num, dR, dG, dB, steps, prescaler);
	while (is_fading(num));
}


void main ()
{			
	uchar i;
	unsigned int j=0;
	unsigned int factor=0; 
    //return;
	init_oscillator();
	init_io_pins(); 


// TODO: check to make sure that timer is stable before going on 

    for ( ; j<255U; j++) {
        // spin
    }
	
	
	init_timer0();
	//TODO: why in the world does this have to be here for the code to work correctly? Something having to do with timer0 not being stable yet or something? 

	for (i=0;i<NUM_LEDS;i++)
	{
		set_rgb(i,100,0,0);
        draw();
	}

	while (1){
        set_rgb(1,0,100,100);
        draw();

	// fade up red in sequence
	for (i=0;i<NUM_LEDS;i++)
	{
		set_fade_blocking(i,1,0,0, 8, 100);
	}
	// fade up blue in reverse sequence
	for (i=0;i<NUM_LEDS;i++)
	{
		set_fade_blocking(NUM_LEDS-i-1,0,0,1, 8, 100);
	}
	// fade down R,B but fade up G slower
	for (i=0;i<NUM_LEDS;i++)
	{
		set_fade_blocking(i,-1,1,-1, 8, 119);
	}
	for (i=0;i<NUM_LEDS;i+=2)
	{
		set_fade(i,1,-1,0, 8, 500);
	}	
	// just green on here

	for (i=0;i<NUM_LEDS;i++)
	{
		if (i%2 ==0)
			set_fade(i,1,-1,0, 8, 500);
		else
			set_fade(i,0,-1,1, 8, 500);
	}	

	// this is sort of like a barrier -- wait for everyone to finish or else things might get out of sync
	for (i=0;i<NUM_LEDS;i++)
	{
		while(is_fading(i));
	}	

	
	while (j=(j+1)%160);
	for (i=0;i<NUM_LEDS;i++)
	{
		if (i%2 ==0)
			set_fade(i,-1,1,1, 8, 500);
		else
			set_fade(i,1,0,-1, 8, 500);
	}	
	
	for (i=0;i<NUM_LEDS;i++)
	{
		while(is_fading(i));
	}	
	while (j=(j+1)%160);
	for (i=0;i<NUM_LEDS;i++)
	{
			set_fade_blocking(i,0,-1,0, 8, 20);
	}	
	for (i=0;i<NUM_LEDS;i++)
	{
			set_fade_blocking(i,-1,0,0, 8, 20);
	}	
	for (i=0;i<NUM_LEDS;i++)
	{
			set_fade_blocking(i,0,0,-1, 8, 20);
	}	
	} // while(1)

}

static void interrupt_draw() __interrupt 1
{
	INTCONbits.TMR0IE = 0; 
	draw();
	INTCONbits.TMR0IF = 0;
	INTCONbits.TMR0IE = 1;
	return;
}

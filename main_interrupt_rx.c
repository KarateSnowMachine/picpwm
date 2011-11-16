	#include <p18f4550.h>
#include "led.h"



#pragma config PBADEN=OFF
#pragma config FOSC=HSPLL_HS, PLLDIV=5, CPUDIV=OSC1_PLL2
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

led_state_t led_colors[NUM_LEDS] = {0};
led_state_t led_target_states[NUM_LEDS] = {0};

void refresh_pwm(uchar refresh_row)
{

	uchar i,c,refresh_led;
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
led_fade_t fades[NUM_LEDS] = {0}; 
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



update_all_fades() 
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
	uchar duty_cycle,x; 
	uchar cycles;
	uchar i;
	unsigned int j=0;
	unsigned int factor=0; 
	init_oscillator();
	init_io_pins(); 


// TODO: check to make sure that timer is stable before going on 


	
	
	init_timer0();
	//TODO: why in the world does this have to be here for the code to work correctly? Something having to do with timer0 not being stable yet or something? 

	while (1){
	for (i=0;i<NUM_LEDS;i++)
	{
		set_rgb(i,0,0,0);
	}

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
		set_fade_blocking(i,-1,1,-1, 8, 300);
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

	}
}

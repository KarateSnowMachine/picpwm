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
	uchar D; // (D)uration only used in target state
}led_state_t;

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
	}
}

void set_rgb(uchar num, uchar r, uchar g, uchar b)
{
	led_colors[num].R = r; 
	led_colors[num].G = g;
	led_colors[num].B = b;
}

void main ()
{			
	uchar duty_cycle,x; 
	uchar cycles;
	uchar i;
	unsigned int j=0;
	init_oscillator();
	init_io_pins(); 

// TODO: check to make sure that timer is stable before going on 
	init_timer0();

	while (1)
	{
	for (i=0; i<NUM_INTENSITY_LEVELS; i++)
	{	
		
		set_rgb(0,i,i,0);
		//set_rgb(4,i,NUM_INTENSITY_LEVELS-i-1,0);
		set_rgb(4,NUM_INTENSITY_LEVELS-1,NUM_INTENSITY_LEVELS-i-1,2);
		set_rgb(7,i,0,NUM_INTENSITY_LEVELS-i-1);

		while(j++%4096); 
	}
	}
	while(1); 
}

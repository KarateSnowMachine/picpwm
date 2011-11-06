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


	
struct led_current_state_t
{
	uchar R;
	uchar G;
	uchar B;
	uchar D; // D is unused in this case? 
};

typedef struct _led_state_to
{
	uchar R:4;
	uchar G:4;
	uchar B:4;
	uchar D:4;
};


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

struct led_current_state_t led_colors[NUM_LEDS] = {0};

void refresh_pwm(uchar refresh_row)
{
	uchar i,c,duty_cycle,refresh_led;
	for (c=0; c<LED_NUM_TYPES; c++)
	{
		for (i=0; i<NUM_COLS; i++)
		{
			refresh_led = refresh_row*NUM_COLS+i;
			duty_cycle = led_colors[c][refresh_led];
			if (pwm_cycle >= (NUM_INTENSITY_LEVELS - duty_cycle))
			{
				set_on(c, refresh_led); 
			}
			else
			{
				set_off(c, refresh_led); 
			}		
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
	led_colors[LED_RED][num] = r; 
	led_colors[LED_GREEN][num] = g;
	led_colors[LED_BLUE][num] = b;
}

void main ()
{			
	uchar duty_cycle,x; 
	uchar cycles;
	uchar i;
	uchar j;
	init_oscillator();
	init_io_pins(); 

// TODO: check to make sure that timer is stable before going on 
	init_timer0();

	for (i=0; i<NUM_INTENSITY_LEVELS; i++)
	{	
		
		set_rgb(0,NUM_INTENSITY_LEVELS-1,0,0);
		set_rgb(1,1,0,0);

		set_rgb(2,0,NUM_INTENSITY_LEVELS-1,0);
		set_rgb(3,0,1,0);		

		set_rgb(4,0,0,NUM_INTENSITY_LEVELS-1);
		set_rgb(5,0,0,1);
		set_rgb(6,NUM_INTENSITY_LEVELS-1,NUM_INTENSITY_LEVELS-1,NUM_INTENSITY_LEVELS-1);
		set_rgb(7,1,1,1);
	}
	while(1); 
}

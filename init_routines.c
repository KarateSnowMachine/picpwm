#include <p18f4550.h>
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
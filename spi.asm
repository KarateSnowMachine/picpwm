	INCLUDE "P18F4550.INC"

	CODE
	GLOBAL spi_write
spi_write: 
	;read in the argument (byte to write) -- see C18 User's Guide page 44
	movlw 	0xff
	movf	PLUSW1, W, A ; WREG = *(FSR1 - 1)
	;now W = byte_to_write

; This code taken from pic18f4550 datasheet page 200 
TransmitSPI:
		BCF PIR1, SSPIF ;Make sure interrupt flag is clear 
		MOVWF SSPBUF, A ;Load data to send into transmit buffer
WaitComplete: ;Loop until data has finished transmitting
		BTFSS PIR1, SSPIF ;Interrupt flag set when transmit is complete
	BRA WaitComplete
	RETURN 

	END



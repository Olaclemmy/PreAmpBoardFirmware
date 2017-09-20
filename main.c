#include <msp430.h> 

//
// Huracan Pre Amp MK III I2C signal routing
//
// v1.0 / 2017-09-19 / Io Engineering / Terje
//
//

/*

Copyright (c) 2015-2017, Terje Io
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

· Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

· Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

· Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdint.h>
#include <stdbool.h>

#include <msp430.h>

#define I2CADDRESS 0x4A

#define SDC BIT6
#define SDA BIT7

#ifdef DAUGTHERBOARD

// Pin mappings
// MSP430: VCC|2.2|2.1|2.0|2.3 |2.4  |2.5 |2.7|2.6  |NC  |1.0 |GND
// AMP:    VCC|~PH|~CD|~TU|TMON|~TC12|~T1 |~T2|~TC21|MONO|~ON |GND
// MONO: not implemented

// P1
#define STDBY    BIT0
#define DABRESET BIT1
// P2
#define AUX      BIT2
#define CD       BIT1
#define TUNER    BIT0
#define TAPE1    BIT5
#define TAPE2    BIT7
#define TC12     BIT4
#define TC21     BIT6
#define TMON     BIT3
#else
// P1
#define STDBY    BIT4
#define DABRESET BIT0
// P2
#define AUX      BIT2
#define CD       BIT1
#define TUNER    BIT0
#define TAPE1    BIT5
#define TAPE2    BIT4
#define TC12     BIT3
#define TC21     BIT7
#define TMON     BIT6

#endif

#define SWITCHCMD   1
#define MUTECMD     2
#define STBYCMD     3
#define DABRESETCMD 4

uint8_t i2cData[3];
volatile uint16_t i2cRXCount;
volatile uint8_t *pi2cData, muteState = 0;

void initI2C (void)
{
	P1SEL  |= SDA|SDC;				// Assign I2C pins to USCI_B0
	P1SEL2 |= SDA|SDC;				// Assign I2C pins to USCI_B0

	IE2 &= ~UCB0TXIE;				// Disable TX interrupt
	UCB0CTL1 |= UCSWRST;			// Enable SW reset
	UCB0CTL0 = UCMODE_3 + UCSYNC;	// I2C Slave, synchronous mode
	UCB0I2COA = I2CADDRESS;			// Own Address is 04Ah
	UCB0CTL1 &= ~UCSWRST;			// Clear SW reset, resume operation
	UCB0I2CIE |= UCSTPIE|UCSTTIE;	// Enable STP (Stop) and STT (Start) interrupts
	IE2 |= UCB0RXIE;	            // Enable RX interrupt
}

void standby (bool standby)
{
	if(standby)
		P1OUT |= STDBY;
	else
		P1OUT &= ~STDBY;
}

void setSwitches (uint8_t cmd)
{
    uint8_t switches = AUX|CD|TUNER|TAPE1|TAPE2|TC12|TC21;

	switch(cmd & 0x03) {

		case 0:
			switches &= ~AUX;
			break;

		case 1:
		case 3:
			switches &= ~CD;
			break;

		case 2:
			switches &= ~TUNER;
			break;
	}

	switch((cmd >> 2) & 0x03) {

		case 1:
			switches &= ~TAPE1;
			switches |= TMON;
			break;

		case 2:
			switches &= ~TAPE2;
			switches |= TMON;
			break;
	}

	switch((cmd >> 4) & 0x03) {

		case 1:
			switches &= ~TC12;
			break;

		case 2:
			switches &= ~TC21;
			break;
	}

	muteState = 0;
	P2OUT = switches;

	standby(false);
}

void main (void)
{
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	BCSCTL1 = CALBC1_1MHZ;			// Use 1MHz calibrated clock
    DCOCTL = CALDCO_1MHZ;			// ...

	P1DIR = 0xFF;					// P1 as outputs
	P1OUT |= DABRESET;              // Release DAB reset

	P2SEL = 0x00;					// P2 all I/O
	P2SEL2 = 0x00;					// ...
	P2DIR = 0xFF;					// P2 as outputs

	P3DIR = 0xFF;					// P3 as outputs
	P3OUT = 0xFF;					// high for stability

	initI2C();

	setSwitches(1); 				// Select CD input as default

	_EINT();						// Enable interrupts

	while(true) {

	    LPM0; 						// Deep sleep MCU, nearly all processing done by interrupts

	    if(!(P1IN & DABRESET)) {    // If resetting DAB radio module
		    _delay_cycles(100000);  // sleep for 100ms and
		    P1OUT |= DABRESET;      // release DAB reset
		}
	}

}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{

	uint8_t ifg = IFG2;

	if(ifg & UCB0TXIFG) {

	}

	if(ifg & UCB0RXIFG) {

		if(i2cRXCount < 3) {
			*pi2cData++ = UCB0RXBUF;
			i2cRXCount++;
		}
	}
}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{

	uint8_t stat = UCB0STAT;

	if(stat & UCSTTIFG) {

		i2cRXCount = 0;
		pi2cData = i2cData;

	} else if((stat & UCSTPIFG) && i2cRXCount) {

		if(i2cRXCount == 2) switch(i2cData[0]) {

			case SWITCHCMD:
				setSwitches(i2cData[1]);
				break;

			case MUTECMD:
				if(i2cData[1]) {
					if(!muteState) {
						muteState = P2IN;                   // Save current signal routing and
						P2OUT |= AUX|CD|TUNER|TAPE1|TAPE2;  // swicth off inputs and
						P2OUT &= ~TMON;                     // tape monitor
					}
				} else if(muteState) {
					P2OUT = muteState;
					muteState = 0;
				}
				break;

			case STBYCMD:
				standby(i2cData[1] != 0);
				break;

		} else if(i2cData[0] == DABRESETCMD) {
		    P1OUT &= ~DABRESET; // Assert DAB reset signal
		    LPM0_EXIT;          // and exit deep sleep
		}

		i2cRXCount = 0;
	}

	UCB0STAT &= ~(UCSTPIFG + UCSTTIFG);		// Clear interrupt flags
}

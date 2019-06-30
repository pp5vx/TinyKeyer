// =========================================================================
// IAMBIC85.c                                                (c)2018 - PP5VX
// =========================================================================
// * Title...: "An Ultra Simple Iambic Keyer with Speed Pot"
// * Hardware: (RISC) ATTiny85 - 6012 bytes (with Bootloader)
//                    ATTiny45 - 2100 bytes (with Bootloader)
//                    ATTiny25 - 2048 bytes (but with **NO** Bootloader)
//                    ATTiny13 - 1024 bytes (...no Bootloader of course)
// * Used RAM: 244 bytes (4%) - ...I am getting old for these things ! (LOL)
// =========================================================================
// Version: 1.0 ( 2012 )                              Filename: [ iambic.c ]
// Author.: Gary Aylward                                   Date: 26 Dec 2012
// Site...: www.garya.org.uk
// =========================================================================
// Version: 2.0b01                                  Filename: [ IAMBIC85.c ]
// Author.: PP5VX (Bone) at pp5vx .--.-. arrl.org          Date: 07 Jan 2018
// Site...: www.qsl.net/pp5vx
// =========================================================================
// Version: 2.0b02                                  Filename: [ IAMBIC85.c ]
// Author.: PP5VX (Bone) at pp5vx .--.-. arrl.org          Date: 29 Jun 2019 
// Added some "confort modifications" (look at new Drawing Schematic)
// => With a lil'ingenuity you can put all this thing on an ATTiny13 !
// =========================================================================
// Briefing:                                      ( BUT YOU MUST READ IT ! )
// =========================================================================
// * Speed Pot: 100K ( LIN)
//   Center ( Viewing from rear ): To ADC1 ( Port B2 )
//   Left   ( Viewing from rear ): To 150K @ 1/4W and VDD 5 VDC
//   Right  ( Viewing from rear ): To 33K  @ 1/4W and GND
// * Voltage Values Range: 0.535V and 2.32V (for a nominal 5 VDC input)
// * ADC1 Values (Speed Port): From 27 to 118 (read below)
//   Speed Range with these ADC1 values are: From 12 wpm to 40 wpm
//   The ADC1 "Reference Voltage" is the VDD Voltage (allways around 5V DC)
//   ==> But the Timing DOES NOT work VERY STABLE **BELOW** 5 VDC !
//   This device DOES NOT WORK with "Button Cell" Batteries (3 VDC)
// =========================================================================
// * Sidetone: From Port P0 to (+) of a (Piezo) Buzzer and perhaps to GND
// =========================================================================
// * TX Key ( Output ): From Port P1 - DIRECT - to GATE (G) of a 2N7000
//                      Source (S) to GND, and Drain (D) to "CW Key Out"
// =========================================================================
// * Iambic Key: DOT  - from (Port) P4 to TIP  of a "TRS Stereo" or P10 
//               DASH - from (Port) P3 to RING of a "TRS Stereo" or P10
// * Note (29 Jun 2019 ):
//   Added a DPDT Switch (a "hardware solution") to Swap the Paddle contacts
// =========================================================================
// * WARNING ! **DO NOT USE** MORE THAN 5 VDC (except for "VIN" below)
//   VIN (on Board): Input from 9 VDC to 16 VDC (for 5 VDC nominal output)
//   5V  (on Board): Input from a REGULATED 5 VDC
//   USB (M3)......: Input from a USB 5 VDC source
// * Note (29 Jun 2019 ):
//   ==> But USE **ONLY ONE** of above sources of DC voltage for POWERING !
// =========================================================================
//
// * Brief WARNING for you ( if you are a Programmer, as is me... ):
// -------------------------------------------------------------------------
// My Programming "taste" don't believe on defaults and standard indentation
// So NEVER use [ CTRL-T ] (on the Arduino IDE) while you are on this code !
// You can get **very worst troubles** of indentation to fully understand it
// -------------------------------------------------------------------------

#include <avr/io.h>
#include <stdint.h>

#define dKEY_PORT	(PORTB)
#define dSIDETONE	(PORTB0)	// Port B0: Sidetone Output
#define dKEY		  (PORTB1)	// Port B1: TX KEY Output
#define dSPEED		(PORTB2)	// Port B2: Speed Pot (Center) Input
#define dDOT_KEY	(PORTB3)	// Port B3: Iambic Key DIT  Input
#define dDASH_KEY	(PORTB4)	// Port B4: Iambic Key DASH Input

// ================================ Timer0 = Sidetone
#define dMAX_COUNT (89)	  // Sidetone Fixed at 700 HZ
#define dGTCCR     (0x00)
#define dTCCR0A    (0x42)
#define dTCCR0B    (0x03)
// ==== Timer1 = State Machine Clock
#define dTCCR1     (0x0E)
#define dWPM_COUNT (159) // 256 - 97

// ====== ADC Control Register Settings
#define dADMUX  (0x21)
#define dADCSRA (0xF6)
#define dADCSRB (0x00)
#define dDIDR0  (1<<ADC1D)

// ====== State Machine: States
#define dIDLE		   0
#define dDOT		   1
#define dDASH		   2
#define dWAIT_DOT	 3
#define dWAIT_DASH 4

// ==== Function Prototype
int main(void);

//===========================  Main
int main (void)
{ uint8_t vCount      = 0;
	uint8_t vThisState  = dIDLE;
	uint8_t vNextState  = dIDLE;
	uint8_t vTimerCount = dWPM_COUNT;
  uint8_t vPotValue;
	DDRB  = 1 << dKEY;                          // Set KEY Pin Output
	PORTB = (1 << dDOT_KEY) | (1 << dDASH_KEY);	// Set Pullup Resistors

	// =========== Timer0 allways generate a 700Hz Tone on OC0A (Pin 5)
  //             But outout it only when as required !
	GTCCR  = dGTCCR;
	OCR0A  = dMAX_COUNT;
	TCCR0A = dTCCR0A;
	TCCR0B = dTCCR0B;

	// ====== Timer1: DOT Time
	TCNT1 = dWPM_COUNT;
	TCCR1 = dTCCR1;

	// ====== Set up ADC
	ADMUX  = dADMUX;
	ADCSRA = dADCSRA;
	ADCSRB = dADCSRB;
	DIDR0  = dDIDR0;

  // ====================== Main Loop ( always True... )
	while (1)
	{ if (TIFR & (1 << TOV1))
		{ // =========== Timer1 Timeout - Reset and move the State Machine to ON
			TIFR |= (1 << TOV1);
			TCNT1 = vTimerCount;
			vThisState = vNextState;
			switch (vThisState)
			{ case dIDLE:      
				case dDOT:       DDRB  |= (1 << dSIDETONE);
					               PORTB |= (1 << dKEY);
					               vNextState = dWAIT_DOT;
                         break;
				case dDASH:      DDRB  |= (1 << dSIDETONE);
					               PORTB |= (1 << dKEY);
					               vCount++;
					               if (vCount >= 3) { vNextState = dWAIT_DASH;
						                                vCount = 0;
						                              }
					                                else vNextState = dDASH;
                         break;
				case dWAIT_DOT:  DDRB  &= ~(1 << dSIDETONE);
					               PORTB &= ~(1 << dKEY);
					               break;
				case dWAIT_DASH: DDRB  &= ~(1 << dSIDETONE);
					               PORTB &= ~(1 << dKEY);
                         break;
			} // end SWITCH
		} // end IF

		// =========================================================== Speed Change
		if ((ADCSRA & (1 << ADIF)) == (1 << ADIF))  // Convert ADC Value and Update
		{ vPotValue = ADCH;				                  // ADC Rresult
			vTimerCount = 256 - vPotValue;	          // New Timer Value
			ADCSRA |= (1 << ADIF);			              // Restart ADC
		}
		// ================================= Poll Key Inputs: Idle or a Wait State
		switch (vThisState)
		{	case dIDLE:       break;
			case dWAIT_DOT:   if ((PINB & (1<<dDASH_KEY))==0) vNextState=dDASH;  // Poll Key Inputs: DASH priority
                        else
			                  if ((PINB & (1<<dDOT_KEY) )==0) vNextState=dDOT; else vNextState=dIDLE;
                        break;
			case dWAIT_DASH:  if ((PINB & (1<<dDOT_KEY) )==0) vNextState=dDOT;   // Poll Key Inputs: DOT priority
				                else
				                if ((PINB & (1<<dDASH_KEY))==0) vNextState=dDASH; else vNextState=dIDLE;
			                  break;
		} // end SWITCH
	} // end WHILE
} // (c)2018, present - PP5VX (V2.0b02 - 29 Jun 2019 )

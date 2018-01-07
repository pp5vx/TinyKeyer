// =========================================================================
// IAMBIC85.c                                                (c)2018 - PP5VX
// =========================================================================
// * Title...: "An Ultra Simple Iambic Keyer with Speed Pot"
// * Hardware: (RISC) ATTiny85 - 6012 bytes ( but with internal Bootup )
//                    ATTiny45 - 2100 bytes ( but with internal Bootup )
// * Used RAM: 244 bytes (4%) - ...I am getting old for these things ! (LOL)
// =========================================================================
// Version: 1.0 ( 2012 )                              Filename: [ iambic.c ]
// Author.: Gary Aylward                                Date...: 26 Dec 2012
// Site...: www.garya.org.uk
// =========================================================================
// Version: 2.0b01                                  Filename: [ IAMBIC85.c ]
// Author.: PP5VX (Bone) at pp5vx .--.-. arrl.org       Date...: 07 Jan 2018
// Site...: www.qsl.net/pp5vx
// =========================================================================

// =========================================================================
// ==> Very brief instructions...                 ( BUT YOU MUST READ IT ! )
// =========================================================================
// * Speed Pot: 100K ( LIN)
//   Center ( Viewing from rear ): To ADC1 ( Port B2 )
//   Left   ( Viewing from rear ): To a 150K @ 1/4W, and to +B (5V DC)
//   Right  ( Viewing from rear ): To a 30K @ 1/4W, and to GND
// * Voltage Values: 0.535V and 2.32V ( for nominal 5V DC input )
// * ADC Values: 27 to 118
// * Speed Range: From about 12 wpm to about 40 wpm
// * Note: ADC Reference is the Supply Voltage, that is allways about 5V DC
//         But the Timing DOES NOT work properly BELOW 5V DC
//         This gadget does not work good with Button Cell Batteries (3V DC)
// =========================================================================
// * Sidetone: From Port P0 to (+) of a Buzzer and from him to GND
// =========================================================================
// * TX Key ( Output ): From Port P1 to a 1N4001 Diode Anode
//                      From Diode Catode to Base of a 2N2222/BC337
//                      Emitter: GND
//                      Collector: To Anode of 1N4001 Diode. Catode to GND
//                                 To onde side of a 100nF Capacitor and GND
//                               To KEY on TX ( Input )
// =========================================================================
// * Iambic Key: DIT  ( tip  on a TRS Stereo or P10 ) - To Port P4
//               DASH ( ring on a TRS Stereo or P10 ) - To Port P3
// =========================================================================
// * WARNING ! **DO NOT USE** MORE THAN 5V DC ( Except for "Vin" below )
//   Vin (on Board): Input from 7V DC to 35 V DC (for 5 V DC Output), is OK
//   5V  (on Board): Input from 5V DC, ditto is OK
//   USB (M3)......: Input from 5V DC, and also is OK
// * But, this gadget DOES NOT operate with Button Cell Batteries (3V DC)
// =========================================================================

// -------------------------------------------------------------------------
// ===---> A brief warning, if you are a Programmer ( as me... ):
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
} // (c)2018 - PP5VX ( V2.0b01 - 07 Jan 2018 )

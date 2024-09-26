/* Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
main function: init for periferals
main loop: check the sleep flag; going to sleep in interrupt does not wake
timmer3 interrupt: cyclic calls of tasks at 5 ms each
funtion to disable interrupt on ir pin in case battery is low
*/
#include "main.h"

// 16Mhz clock
#define CLOCK 16000000L
// Divide by 12
#define T0CLOCK ((CLOCK)/12L)
// Per milisecond
#define T0_1MS ((T0CLOCK)/1000L)

#define DEBUG 0		//1= led blink

static void sleep_on();
static void enable_wake_ir(uint8_t stat);


volatile power_state_type power = ON;

volatile battery_state_type battery_state = BATTERY_OK;
volatile battery_state_type battery_state_old = BATTERY_OK;

uint8_t main_counter = 0;

enum
{
MAIN_START,
MAIN_START_WAIT,
MAIN_BATT,
MAIN_BATT_WAIT,
MAIN_NVM,
MAIN_NORMAL,
MAIN_BATTERY_LOW,
MAIN_BATTERY_LOW_WAIT,
} main_state = MAIN_START;

void main() {

	// Set pins in old-skool Quasi Bidirectional mode
	P0M1 = 0x14;	//P04 as input; P03 as input
	P0M2 = 0;
	P1M1 = 0;
	P1M2 = 0x18;	// 14 as push-pull ; 13 as push-pull
	P3M1 = 0;
	P3M2 = 0;
	
	SFR_PAGE(1);
	P0S= (1<<4);	//schmitt trigger p04
	P0S= (1<<3);	//schmitt trigger p03
	SFR_PAGE(0);

//pin interrupt
	/*
	P04 - ir input
	PIT45 = 1; bit 6; pins p04 and p05 edge trigger - using p04
	PIPS = 0; bits 1:0; select port 0 
	*/
	PICON = (1<<6) | (1<<5); //edge trigger on on P45 and P3
	PINEN = (1<<4) | (1<<3);	//fall edge on p04 and P03
	//PIPEN = (1<<4);	//rise edge on p04
//ir settings
//timer capture setting - timmer 2 input capture

	CM_RL2 = 0;		//auto reload
	/*
	T2DIV = 3; bits 6:4; divider is 512 to make period 32us
	CAPCR = 1; bit 3; capture auto crear
	LDTS = 1; bits 1:0; reload on input capture 0 event
	*/
	T2MOD = (1<<7) |(1<<6) |(1<<5) |(1<<4) | (1<<3) | (1<<0);	//div=1/512; capcr=1; LDTS=0b01

	CAPCON1 = (1<<1);	//input cap 0 rise and fall
	CAPCON3 = (1<<2);	//capture channel 0 on pin P04
	CAPCON0 = (1<<4);	//enable input capture 0
	CAPCON2 = (1<<4);	// noise filter
	
//timer 3 setup - cyclic interrupt
	SET_FIELD(T3CON, TF3, 0);	//clear int flg
	SET_FIELD(T3CON, T3PS, 0b111);	//presc 128
	RL3 = 130;	//will count 125 clocks to get 1ms
	RH3 = 255;

	EIE1 = 2;	//enable timer 3 interrupt
//interrupts
	/*
	EPI = 1; bit 1; enable pin interrupt
	ECAP = 1; bit 2; enable input capture iterrupt
	*/
	EIE = EIE_EPI_BIT | EIE_ECAP_BIT;	
	EA = 1;		//enable interrupts

//timers start
	TR2 = 1;	//start timmer 2 for ir
	
	SET_FIELD(T3CON, TR3, 1);	//timer 3 start

//pin states
	P12 = 1;	//D2 off - do remove

	P14 = 0;
outmux_pwm_init();
	
	//ws_power(1);
	for (;;)
	{
		//call sleep here because it does not wake if called from interrupt
		if(power == OFF)
		{
			sleep_on();
		}
	}
}
/*
5ms tasks
one has the state machine - to make wait states
others have the mainfuncions
mainfunctions called selectively, decide using current state what is called cyclicly
*/
void ISR_Timer3(void) __interrupt(16)
{
	//1ms calls
	static uint8_t count = 0;
	

	count++;
	/* 5 ms tasks */
	if(count == 1)
	{
#if(DEBUG == 1)
		static uint16_t ledCount=0;
		//if(power == ON)
		{
		ledCount++;
		if(ledCount == 20)
		{
			P12 = 0;	//D2 on
		}
		if(ledCount == 40)
		{
			P12 = 1;	//D2 off
			ledCount=0;
		}
		}
#endif
	}
	if(count == 2)
	{
		ir_mainfunction(&power);
		if(power == ON)
		{
			switch(main_state)
			{
				case MAIN_START:
				{
					main_counter=0;
					main_state = MAIN_START_WAIT;
					break;
				}
				case MAIN_START_WAIT:
				{
					main_counter++;
					if(main_counter == 10)
					{
						main_counter=0;
						if(battery_state == BATTERY_OK)
						{
							main_state = MAIN_BATT;
						}
						else
						{
							main_state = MAIN_BATTERY_LOW;
						}
					}
					break;
				}
				case MAIN_BATT:
				{
					battery_showstate();
					main_state = MAIN_BATT_WAIT;
					break;
				}
				case MAIN_BATT_WAIT:
				{
					main_counter++;
					if(main_counter == 200)
					{
						main_counter=0;
						main_state = MAIN_NVM;
					}
					break;
				}
				case MAIN_NVM:
				{
					outmux_init();
					main_state = MAIN_NORMAL;
					break;
				}
				case MAIN_NORMAL:
				{
					
					if(battery_state == BATTERY_NOT_OK)
					{
						main_state = MAIN_BATTERY_LOW;
						//power on ws and reset counter
						main_counter=0;
						ws_power(1);
					}
					break;
				}
				case MAIN_BATTERY_LOW:
				{//turn on counter for ws
					main_counter++;
					if(main_counter == 10)
					{//reset counter and show state of battery
						main_counter=0;
						battery_showstate();
						main_state = MAIN_BATTERY_LOW_WAIT;
					}
					break;
				}
				case MAIN_BATTERY_LOW_WAIT:
				{//wait 0.5 sec and go to power off
					main_counter++;
					if(main_counter == 100)
					{
						main_counter=0;
						power = PRE_OFF;
					}
					break;
				}
			}

		}
		if(power == PRE_OFF)
		{
			//reset main state machine
			main_state = MAIN_START;
		}
	}
	if(count == 3)
	{
		//power management
		power_mangage_mainf(&power);
		//write outputs
		outmux_mainfunction(&power);
	}
	if(count == 4)
	{
		battery_mainfuncion(&battery_state);
		if((battery_state == BATTERY_OK) && (battery_state_old == BATTERY_NOT_OK))
		{//battery was low but now is ok - enable wake by ir
			enable_wake_ir(1);
		}
		if((battery_state == BATTERY_NOT_OK) && (battery_state_old == BATTERY_OK))
		{//battery is low - disable ir wake
			enable_wake_ir(0);
		}
		battery_state_old = battery_state;
	}
	if(count == 5)
	{
		buttons_mainfunction(&power);
		count = 0; // reset counter
		
	}

}

void IR_ISR_PIN(void) __interrupt(7)	//pin interrupt
{
	if(PIF & (1<<4))	//interrupt on p04 - ir
	{
		PIF &=  ~(1<<4);	//reset flag
		if(power == OFF)
		{
			power = PRE_ON;
		}
	}
	if(PIF & (1<<3))	//interrupt on p04 - ir
	{
		PIF &=  ~(1<<3);	//reset flag
		if(power == OFF)
		{
			power = PRE_ON;
		}
	}
}

static void sleep_on()
{
	SET_FIELD(PCON,PD,1);
}

static void enable_wake_ir(uint8_t stat)
{
	if(stat == 1)
	{
		PINEN |= (1<<4);	//fall edge on p04 enable
	}
	else
	{
		PINEN &= ~(1<<4);	//fall edge on p04 disable
	}
}


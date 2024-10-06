/*
masured with variable supply at 5V: about 3 mA diference for sleep/awake
	measured 0mA during sleep with only uc and power led
used the voltage value to light some leds to indicate battery charge state
*/
#include "main.h"

#define Set_IAPEN TA=0Xaa;TA=0x55;CHPCON|=1
#define set_IAPGO TA=0Xaa;TA=0x55;IAPTRG|=1
#define Clr_IAPEN TA=0Xaa;TA=0x55;CHPCON&=~1

#define VOLTAGE_ADJUST 0	//100 = 0.1V
#define BAT_LOW 32	//3.2V
#define TIME_1S 200
#define TIME_25MS 5

__xdata static uint16_t Bandgap_Voltage,VDD_Voltage;

__xdata static ws_colorIndex_type array2[16];

/* voltage steps for show state
	35 = 3.5 vols
	value from readVoltage() function is 35 = 3.5v
 */
static const uint8_t voltageTable[8]={33,34,35,36,37,38,39,40};


uint8_t battery_readVoltage();

/* is called at 5ms */
void battery_mainfuncion(battery_state_type *b)
{
	/*
	return a good/not status to main
	read voltage time:
		- once every ~1minute if last value is ok
		- 25 ms if last status is low
	*/
	
	static uint8_t probe_counter = 0;
	static uint8_t volt = BAT_LOW;
	static uint8_t timer_val = TIME_25MS;
	if(probe_counter == 0)
	{
		//update the voltage read
		volt = battery_readVoltage();
	}
	/* increment and reset */
	probe_counter++;
	if(probe_counter >= timer_val)
	{
		probe_counter = 0;
	}
	/* return val */
	if(volt <= BAT_LOW)
	{
		//return nok
		*b = BATTERY_NOT_OK;
		timer_val = TIME_25MS;
	}
	else
	{
		//return ok
		*b = BATTERY_OK;
		timer_val = TIME_1S;
	}
}

void battery_showstate()
{
	uint8_t v = battery_readVoltage();
	uint8_t i;
	for(i = 0; i < 8; i++)
	{
		if(v > voltageTable[i])
		{
			array2[i] = WS_GREEN_0;
		}
		else
		{
			array2[i] = WS_RED_0;
		}
	}
	while(i<16)
	{
		array2[i]=WS_OFF;
		i++;
	}
	ws_array(array2,0);
}
/* return voltage: 35 = 3.5V*/
uint8_t battery_readVoltage()
{
float bgvalue;
uint8_t ozc;
uint16_t Bandgap_Value;
//read bandgap
uint8_t BandgapHigh,BandgapLow;
Set_IAPEN; // Enable IAPEN
IAPAL = 0x0C;
IAPAH = 0x00;
IAPCN = 0x04;
set_IAPGO; // Trig set IAPGO
BandgapHigh = IAPFD;
IAPAL = 0x0d;
IAPAH = 0x00;
IAPCN = 0x04;
set_IAPGO; // Trig set IAPGO
BandgapLow = IAPFD;
BandgapLow = BandgapLow&0x0F;
Clr_IAPEN; // Disable IAPEN
Bandgap_Value = (BandgapHigh<<4)+BandgapLow;
//Bandgap_Voltage = 3072/(0x1000/Bandgap_Value);
Bandgap_Voltage = Bandgap_Value/1.33333;

//read vdd
//Enable_ADC_BandGap;
	//ADC setup
	ADCCON1 |= 1;	//ADCEN = 1
	ADCHS3 = 1;	//select bandgap
	ADCHS2 = 0;
	ADCHS1 = 0;
	ADCHS0 = 0;
for (ozc=0;ozc<0x03;ozc++)
	{
	ADCF = 0;
	ADCS = 1;
	while(ADCF == 0);
	}
ADCF = 0;
ADCS = 1;
while(ADCF == 0);
bgvalue = (ADCRH<<4) + ADCRL;
VDD_Voltage = (0Xfff/bgvalue)*Bandgap_Voltage;

//voltage
return (uint8_t)((VDD_Voltage-VOLTAGE_ADJUST)/100);
}

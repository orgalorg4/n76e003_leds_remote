/*
manage trasitions between on and sleep depending on the inputs
for ir: any ir signal wake the micro, but only one code make the transition to ON state
	if the expected code is not received then go back to sleep
power button just wake
*/

#include "main.h"


volatile power_state_type power_old = OFF;
uint8_t power_counter = 0;


void power_mangage_mainf(power_state_type *power)
{
	if((*power == ON) && (power_old != ON))
	{//conditions ok to turn on
		ws_power(1);
		power_old = *power;
	}
	if((*power == PRE_ON) && (power_old == OFF))
	{//interrupt received; wait for confirnation of correct button press
		//reset counter
		power_counter = 0;
		power_old = *power; 
	}
	if(*power == PRE_ON)
	{//wait for confirnation of correct button press
		//start a counter, about 25ms; if power button is pressed, then var power will have val ON
		power_counter ++;
		if(power_counter >= 50)	//wait for power button press on remote; if not received then sleep 
		{
			*power = OFF;
		}
		power_old = *power; 
	}
	if((*power == PRE_OFF) && (power_old == ON))
	{
		//off command from remote writes pre off to power var
		//reset conter
		power_counter = 0;
		power_old = *power; 
	}
	if(*power == PRE_OFF)
	{
		//count a few cycles before turning off to wirte nvm from main
		//if ir gets noise while here then state will go to pre on, count there and then off
		power_counter ++;
		if(power_counter >= 40) 
		{
			*power = OFF;
		}
		power_old = *power; 
	}
	if(*power == OFF)
	{
		power_old = *power;
		outmux_out_set(OUTMUX_NONE); 
	}
}


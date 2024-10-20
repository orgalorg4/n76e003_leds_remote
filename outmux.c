/*
change the output leds
init for pwm
set duty for pwm
variables set from other functions; used here to set the leds
write the nvm for sleep
read nvm at wakeup
brightness function can be called with define for up/down to adjust brightness, or the bightness index
colour changing function can called with next/previous or the colour index

WS usb define left unused - can be used to add code for another ws output
*/
#include "main.h"

#define PWM_PERIOD 2000//255		//500hz freq
#define PWM_DUTY_MAX PWM_PERIOD 

typedef enum{
	PWM_SHOW_NONE,
	PWM_SHOW_INIT_1,
	PWM_SHOW_INIT_2,
	PWM_SHOW_SHOW,
	PWM_SHOW_WAIT,
	PWM_SHOW_OFF,
}ws_show_pwm_type;

__xdata static ilum_data_type ilum_data, ilum_data_old;

__xdata static uint8_t outmux_initdone = 0;

__xdata static ws_show_pwm_type ws_show_pwm_state = PWM_SHOW_NONE;
__xdata static uint8_t ws_show_pwm_count = 0;
__xdata static ws_colorIndex_type array3[16];

//pwm1 on pin 1.1
void outmux_pwm_init()
{
		//PWM setup
	CLRPWM = 1;	//clear pwm timer
	while(CLRPWM != 0);	//wait for pwm timer to clear
	PWMCON1 |= 4; //prescaler = 16
	SET_FIELD(CKCON, PWMCKS, 0);	//clock source for pwm is Fsys
	PIOCON0 = 1<<1;	//P1.1/PWM1 pin functions as PWM1 output.
	PWMPH = (PWM_PERIOD >> 8);
	PWMPL = (PWM_PERIOD & 0xff);	//500hz freq

	//start pwm
	PWMRUN = 1;
	//set duty
	PWM1H = 0x00;
	PWM1L = 0x00;

	LOAD = 1;	//load period and duty
	while(LOAD){__asm__ ("nop");}
	PWMRUN = 0;
}
void pwm_duty(uint8_t pwm_duty_index)
{
	uint32_t aux;
	aux = (uint32_t)gamma_table[pwm_duty_index] * PWM_DUTY_MAX / 256;
	PWM1H = ((aux >> 8));
	PWM1L = (aux & 0xff);
	
	LOAD = 1;	//load period and duty
}

void outmux_init()
{
	//read nvm and set the output, colour and brightness
	eeprom_read_data(&ilum_data);
	eeprom_read_data(&ilum_data_old);
	if((ilum_data_old.outmux_out == OUTMUX_WS_BUILTIN) || (ilum_data_old.outmux_out == OUTMUX_WS_USB))
	{//ws will be on from the bat show
		ws_oneColor(&(ilum_data.ws));
	}
	if(ilum_data.outmux_out == OUTMUX_PWM_USB)
	{
		ws_power(0);	//turn off ws main
		PWMRUN = 1;	//start pwm
		pwm_duty(ilum_data.pwm);
	}
	outmux_initdone = 1;
}

void outmux_out_set(outmux_out_type out)
{
	if(OUTMUX_NEXT == out)
	{
		switch(ilum_data.outmux_out)
		{
			case OUTMUX_WS_BUILTIN:
				ilum_data.outmux_out = OUTMUX_PWM_USB;
				break;
			case OUTMUX_PWM_USB:
				ilum_data.outmux_out = OUTMUX_WS_BUILTIN;
				break;
		}
	}
	else
	{
		ilum_data.outmux_out = out;
	}
}

void outmux_out_get(outmux_out_type *out)
{
	*out = ilum_data.outmux_out;
}

void outmux_set_bright(uint8_t br)
{
	switch(ilum_data.outmux_out)
	{
		case OUTMUX_WS_BUILTIN:
		case OUTMUX_WS_USB:
			if(br > WS_BRIGH_SIZE)
			{
				if(br == WS_BRIGH_PLUS)
				{
					ilum_data.ws.brigInd++;
					if(ilum_data.ws.brigInd > WS_BRIGH_LIMIT){ilum_data.ws.brigInd = WS_BRIGH_LIMIT;}
				}
				if(br == WS_BRIGH_MINUS)
				{
					ilum_data.ws.brigInd--;
					if(ilum_data.ws.brigInd >= WS_BRIGH_SIZE){ilum_data.ws.brigInd = 0;}
				}
			}
			if(br < WS_BRIGH_SIZE)
			{
				if(br > WS_BRIGH_LIMIT)
				{
					ilum_data.ws.brigInd = WS_BRIGH_LIMIT;
				}
				else
				{
					ilum_data.ws.brigInd = br;
				}
			}
			break;
		case OUTMUX_PWM_USB:
			ws_show_pwm_state = PWM_SHOW_INIT_1;
			if(br > WS_BRIGH_SIZE)
			{
				if(br == WS_BRIGH_PLUS)
				{
					ilum_data.pwm++;
					if(ilum_data.pwm >= WS_BRIGH_SIZE){ilum_data.pwm = WS_BRIGH_SIZE-1;}
				}
				if(br == WS_BRIGH_MINUS)
				{
					ilum_data.pwm--;
					if(ilum_data.pwm >= WS_BRIGH_SIZE){ilum_data.pwm = 0;}
				}
			}
			if(br < WS_BRIGH_SIZE)
			{
				ilum_data.pwm = br;
			}
			break;
	}
}

void outmux_set_ws_colour(ws_colorIndex_type col)
{
	if(ilum_data.outmux_out == OUTMUX_WS_BUILTIN)
	{
		if(col > WS_COL_SIZE)
		{
			if(col == WS_COL_NEXT)
			{
				ilum_data.ws.colInd++;
				if(ilum_data.ws.colInd > WS_WHITE) ilum_data.ws.colInd = 0;
			}
			if(col == WS_COL_PREV)
			{
				ilum_data.ws.colInd--;
				if(ilum_data.ws.colInd > WS_WHITE) ilum_data.ws.colInd = WS_WHITE;
			}
		}
		if(col < WS_COL_SIZE)
		{
			ilum_data.ws.colInd = col;
		}
	}
}

void outmux_mainfunction(power_state_type *power)
{
	// delay after power on ws before write data - just use for all outputs
	static uint8_t delay_counter;
	static uint8_t output_change = 0;
	uint8_t i;
	if(ilum_data_old.outmux_out != ilum_data.outmux_out)
	{
		ilum_data_old.outmux_out = ilum_data.outmux_out;
		switch(ilum_data.outmux_out)
		{//power on the selected output; power off the rest
			case OUTMUX_WS_BUILTIN:
				ws_power(1);
				PWM1H = 0x0;
				PWM1L = 0x0;
				LOAD = 1;	//load period and duty
				while(LOAD){__asm__ ("nop");}
				PWMRUN = 0;	//stop pwm
				break;
			case OUTMUX_WS_USB:
				ws_power(0);
				break;
			case OUTMUX_PWM_USB:
				ws_power(0);
				PWMRUN = 1;	//start pwm
				break;
			case OUTMUX_NONE:
				ws_power(0);
				PWM1H = 0x0;
				PWM1L = 0x0;
				LOAD = 1;	//load period and duty
				PWMRUN = 0;	//stop pwm
				break;
		}
		//set delay to 50ms
		delay_counter = 10;
		output_change = 1;
	}
	if(delay_counter > 0)
	{//wait state
		delay_counter--;
	}
	else
	{//delay time elapsed - set output
		switch(ilum_data.outmux_out)
		{//set colour/brightness of leds
			case OUTMUX_WS_BUILTIN:
			{
				if((ilum_data.ws.colInd != ilum_data_old.ws.colInd) || (ilum_data.ws.brigInd != ilum_data_old.ws.brigInd) || (output_change == 1))
				{
					output_change = 0;
					ilum_data_old.ws.colInd = ilum_data.ws.colInd;
					ilum_data_old.ws.brigInd = ilum_data.ws.brigInd;
					ws_oneColor(&(ilum_data.ws));
				}
			}
				break;
			case OUTMUX_WS_USB:
				
				break;
			case OUTMUX_PWM_USB:
				if((ilum_data.pwm != ilum_data_old.pwm) || (output_change == 1))
				{
					output_change = 0;
					ilum_data_old.pwm = ilum_data.pwm;
					pwm_duty(ilum_data.pwm);
				}
				break;
		}
	}
	
	switch(ws_show_pwm_state)
	{
		case PWM_SHOW_INIT_1:
			ws_power(1);
			ws_show_pwm_count = 5;
			ws_show_pwm_state = PWM_SHOW_INIT_2;
			break;
		case PWM_SHOW_INIT_2:
			ws_show_pwm_count--;
			if(ws_show_pwm_count == 0)
				ws_show_pwm_state = PWM_SHOW_SHOW;
			break;
		case PWM_SHOW_SHOW:
			for(i=0;i<ilum_data.pwm/2+1;i++)
			{
				array3[i] = WS_RED_4;
			}
			for(i;i<16;i++)
			{
				array3[i] = WS_OFF;
			}
			ws_array(array3,0);
			ws_show_pwm_count = 100;
			ws_show_pwm_state = PWM_SHOW_WAIT;
			break;
		case PWM_SHOW_WAIT:
			ws_show_pwm_count--;
			if(ws_show_pwm_count == 0)
				ws_show_pwm_state = PWM_SHOW_OFF;
			break;
		case PWM_SHOW_OFF:
			ws_power(0);
			ws_show_pwm_state = PWM_SHOW_NONE;
			break;
	}
	
	if(*power == PRE_OFF)
	{
		if(outmux_initdone == 1)
		{
			//write nvm
			eeprom_write_data(&ilum_data_old);
			outmux_initdone = 0;
		}
		ws_show_pwm_state = PWM_SHOW_NONE;
		ws_power(0);
		PWM1H = 0x0;
		PWM1L = 0x0;
		LOAD = 1;	//load period and duty
	}
}

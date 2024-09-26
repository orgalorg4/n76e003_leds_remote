/*
read 3 buttons:power, up, down
up/down do brightness
power + up changes colour on ws leds
power + down changes the output led
up + down does nothing, but could be used
*/

#include "main.h"


#define BTN_0_PIN P03
#define BTN_1_PIN P02
#define BTN_2_PIN P01

#define BTN_DEBOUCE_CYCLES 10	//debounce 50ms

#define BTN_PRESS 0
#define BTN_RELESE 1


enum{
	BTN_0_GO=0,
	BTN_1_PLUS,
	BTN_2_MINUS,
	BTN_SIZE,
};
typedef struct {
	uint8_t debounce_cnt;
	uint8_t debounce_state :1;
	uint8_t out :1;
	uint8_t out_old :1;
	uint8_t read :1;
}button_data_t;

__xdata volatile button_data_t button_arr[BTN_SIZE]={
/* initial states for buttons; debounce state 0 will trigger a debouncing but it will not change the out state as it is already release */
{0,0,BTN_RELESE,BTN_RELESE},
{0,0,BTN_RELESE,BTN_RELESE},
{0,0,BTN_RELESE,BTN_RELESE},
};

static volatile uint8_t btn_flag = 0;

static void buttons_update();

void buttons_mainfunction(power_state_type *power)
{
	uint8_t i;
	//get the new states
	buttons_update();
	
	//process buttons
	//light change
		//signle button
	if(button_arr[BTN_0_GO].out == BTN_RELESE)
	{//actions for one or both adjust buttons
		if(((button_arr[BTN_1_PLUS].out == BTN_PRESS) && (button_arr[BTN_2_MINUS].out == BTN_PRESS) && (button_arr[BTN_2_MINUS].out_old == BTN_RELESE))
		||((button_arr[BTN_2_MINUS].out == BTN_PRESS) && (button_arr[BTN_1_PLUS].out == BTN_PRESS) && (button_arr[BTN_1_PLUS].out_old == BTN_RELESE)))
		{//both adjust buttons are pressed
			//action - colour transitions?
		}
		else if((button_arr[BTN_1_PLUS].out == BTN_PRESS) && (button_arr[BTN_1_PLUS].out_old == BTN_RELESE))
		{//press edge on plus button
			//increase brightness
			outmux_set_bright(WS_BRIGH_PLUS);
		}
		else if((button_arr[BTN_2_MINUS].out == BTN_PRESS) && (button_arr[BTN_2_MINUS].out_old == BTN_RELESE))
		{//press edge on minus button
			//decrease brightness
			outmux_set_bright(WS_BRIGH_MINUS);
		}
	}
		//2 button combo
	if(button_arr[BTN_0_GO].out == BTN_PRESS)
	{
		if((button_arr[BTN_1_PLUS].out == BTN_PRESS) || (button_arr[BTN_2_MINUS].out == BTN_PRESS))
		{//power button is press with another - count relese edge on sleep button to not go off
			btn_flag = 0;
		}
		if((button_arr[BTN_1_PLUS].out == BTN_PRESS) && (button_arr[BTN_1_PLUS].out_old == BTN_RELESE))
		{//power button is pressed and press edge on plus button
			//some action - next colour
			outmux_set_ws_colour(WS_COL_NEXT);
		}
		if((button_arr[BTN_2_MINUS].out == BTN_PRESS) && (button_arr[BTN_2_MINUS].out_old == BTN_RELESE))
		{//power button is pressed and press edge on minus button
			//some action - change output
			outmux_out_set(OUTMUX_NEXT); 
		}
	}
	
	//power button
	if((*power == PRE_ON) && (button_arr[BTN_0_GO].out == BTN_PRESS))
	{//the press during sleep
		*power = ON;
	}
	if((button_arr[BTN_0_GO].out == BTN_RELESE) && (button_arr[BTN_0_GO].out_old == BTN_PRESS))
	{//negative edge
		if(btn_flag == 0)
		{//falling edge of press during sleep - prevent going to sleep for the release
			btn_flag = 1;
		}
		else if((btn_flag == 1) && (*power == ON))
		{//the press for sleep - use the falling edge so btn out is release for the next cycle
			*power = PRE_OFF;
			btn_flag=0;
		}
	}
	if((button_arr[BTN_0_GO].out == BTN_RELESE) && (button_arr[BTN_0_GO].out_old == BTN_RELESE))
	{//no transition
		if((*power == ON) && (btn_flag == 0))
		{//wake from ir - set flag so btn can be used for sleep
			btn_flag = 1;
		}
		if((*power == PRE_OFF) && (btn_flag == 1))
		{//sleep from ir - set flag so btn can be used for wake
			btn_flag = 0;
		}
	}
	
	
	//write the old states
	for(i = 0; i < BTN_SIZE; i++)
	{
		button_arr[i].out_old = button_arr[i].out;
	}
}

static void buttons_update()
{
	uint8_t i;
	
	//read button/s
	button_arr[BTN_0_GO].read = BTN_0_PIN;
	button_arr[BTN_1_PLUS].read = BTN_1_PIN;
	button_arr[BTN_2_MINUS].read = BTN_2_PIN;
		
	for(i = 0; i < BTN_SIZE; i++)
	{
		//do debounce
		if(button_arr[i].read != button_arr[i].debounce_state)
		{
			button_arr[i].debounce_cnt = 0;
			button_arr[i].debounce_state = button_arr[i].read;
		}
		if(button_arr[i].debounce_state != button_arr[i].out)
		{
			button_arr[i].debounce_cnt++;
			if(button_arr[i].debounce_cnt >= BTN_DEBOUCE_CYCLES)
			{
				button_arr[i].out = button_arr[i].debounce_state;
			}
		}
	}
}
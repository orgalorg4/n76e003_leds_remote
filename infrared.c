/* 
decode infrared codes in NEC protocol
using input caputre compare interrupt on timmer2
count on increments of 32us
at transition on pin interrupt is executed, read the capture time and interpred data
defines for the buttons on 2 similar remotes
*/

#define REMOTE_R1 1
#define REMOTE_R2 2
#define REMOTE_DEF REMOTE_R1

#include "main.h"
#if(REMOTE_DEF == REMOTE_R2)
#include "nec_r2_map.h"
#elif(REMOTE_DEF == REMOTE_R1)
#include "nec_r1_map.h"
#endif


//ir
//times in 32 us units
//actual times should be(us): head1:9000; head2:4500; space:560; mark1:1678; mark0:560; but chineze remote;
#define IR_T_HEAD_MIN  125//4000u/32
#define IR_T_HEAD_MAX  300//9600u/32
#define IR_T_SPACE_MIN 10//320u/32
#define IR_T_SPACE_MAX 25//800u/32
#define IR_T_MARK1_MIN 40//1280u/32
#define IR_T_MARK1_MAX 60//1920u/32
#define IR_T_MARK0_MIN 14//512u/32
#define IR_T_MARK0_MAX 25//800u/32
//states
#define IR_IDLE  1
#define IR_HEAD  2
#define IR_SPACE 3
#define IR_MARK  4
#define IR_FAIL  5
#define IR_END   6
//end of ir
//ir
typedef struct {
	uint8_t state;
	uint16_t time_lev;
	uint32_t decoded_val;
	uint8_t decoded_flag;
	uint8_t bit_count;
}ir_data_t;
//ir end
//ir
volatile ir_data_t ir_data = {IR_IDLE,0,0,0,0};
//ir end


//vars for colours
ws_colorIndex_type array[8]={0,1,2,3,4,5,6,7};
uint8_t tempColInd = 7;


void ir_mainfunction(power_state_type *power)
{
	if(ir_data.decoded_flag == 1)
	{
		ir_data.decoded_flag = 0;
		/* if power is off then check only the power on button;
		   if power is on then do stuff */
#if(REMOTE_DEF == REMOTE_R2)
		if(*power == PRE_ON)
		{
			if(ir_data.decoded_val == NEC_ON)
			{
				*power = ON;
			}
		}
		else if(*power == ON)
		{
			switch(ir_data.decoded_val)
			{
				case NEC_OFF:
				{
					*power = PRE_OFF;
					break;
				}
				case NEC_RED:
				{
					outmux_set_ws_colour(WS_RED_0);
					break;
				}
				case NEC_GREEN:
				{
					outmux_set_ws_colour(WS_GREEN_0);
					break;
				}
				case NEC_BLUE:
				{
					outmux_set_ws_colour(WS_BLUE_0);
					break;
				}
				case NEC_WHITE:
				{
					outmux_set_ws_colour(WS_WHITE);
					break;
				}
				case NEC_BRIGHTNESS_UP:
				{
					outmux_set_bright(WS_BRIGH_PLUS);
					break;
				}
				case NEC_BRIGHTNESS_DOWN:
				{
					outmux_set_bright(WS_BRIGH_MINUS);
					break;
				}
				case NEC_SMOOTH:
				{
					uint8_t i;
					for(i=0;i<7;i++)
					{
						array[i] = array[i+1]; 
					}
					tempColInd ++;
					if(tempColInd >= WS_COL_SIZE) tempColInd = 0;
					array[7] = tempColInd;
					ws_array(array,0);
					break;
				}
				case NEC_FLASH:
				{
					outmux_out_set(OUTMUX_NEXT); 
					break;
				}
			}
		}
#endif
#if(REMOTE_DEF == REMOTE_R1)
		if(*power == PRE_ON)
		{
			if(ir_data.decoded_val == NEC_R1_ONOFF)
			{
				*power = ON;
			}
		}
		else if(*power == ON)
		{
			switch(ir_data.decoded_val)
			{
				case NEC_R1_ONOFF:
				{
					*power = PRE_OFF;
					break;
				}
				case NEC_R1_T1:
				{
					outmux_set_ws_colour(WS_RED_0);
					break;
				}
				case NEC_R1_T2:
				{
					outmux_set_ws_colour(WS_GREEN_0);
					break;
				}
				case NEC_R1_T3:
				{
					outmux_set_ws_colour(WS_BLUE_0);
					break;
				}
				case NEC_R1_T0:
				{
					outmux_set_ws_colour(WS_WHITE);
					break;
				}
				case NEC_R1_VOLUME_UP:
				{
					outmux_set_bright(WS_BRIGH_PLUS);
					break;
				}
				case NEC_R1_VOLUME_DOWN:
				{
					outmux_set_bright(WS_BRIGH_MINUS);
					break;
				}
				case NEC_R1_EQ:
				{
					uint8_t i;
					for(i=0;i<7;i++)
					{
						array[i] = array[i+1]; 
					}
					tempColInd ++;
					if(tempColInd >= WS_COL_SIZE) tempColInd = 0;
					array[7] = tempColInd;
					ws_array(array,0);
					break;
				}
				case NEC_R1_NEXT:
				{
					outmux_set_ws_colour(WS_COL_NEXT);
					break;
				}
				case NEC_R1_PREVIOUS:
				{
					outmux_set_ws_colour(WS_COL_PREV);
					break;
				}
				case NEC_R1_MODE:
				{
					outmux_out_set(OUTMUX_NEXT); 
					break;
				}
			}
		}
#endif
	}
}




void IR_ISR_IR(void) __interrupt(12)	//tim2 capture for ir
{	
	if(CAPCON0 & 1)	//input compare 0
	{
		CAPCON0 &= ~1;
		ir_data.time_lev = (uint16_t)(C0H << 8) | C0L;	//take new time

		switch(ir_data.state)
		{
		case IR_IDLE:
			if((ir_data.time_lev > IR_T_HEAD_MIN) && (ir_data.time_lev < IR_T_HEAD_MAX))
				{ir_data.state = IR_HEAD;}
			break;
		case IR_HEAD:  //2 header times
			if((ir_data.time_lev > IR_T_HEAD_MIN) && (ir_data.time_lev < IR_T_HEAD_MAX))
				{ir_data.state = IR_SPACE;}
			else
				ir_data.state = IR_IDLE;
			break;
		case IR_SPACE:
			if((ir_data.time_lev > IR_T_SPACE_MIN) && (ir_data.time_lev < IR_T_SPACE_MAX))
				{ir_data.state = IR_MARK;}
			else
				ir_data.state = IR_FAIL;
			break;
		case IR_MARK:
			ir_data.decoded_val = ir_data.decoded_val << 1;
			ir_data.bit_count++;
			if((ir_data.time_lev > IR_T_MARK0_MIN) && (ir_data.time_lev < IR_T_MARK0_MAX))
			{
				if(ir_data.bit_count < 32)
					ir_data.state = IR_SPACE;
				else
					ir_data.state = IR_END;
			}
			else if((ir_data.time_lev > IR_T_MARK1_MIN) && (ir_data.time_lev < IR_T_MARK1_MAX))
			{
				ir_data.decoded_val |= 1;
				if(ir_data.bit_count < 32)
					ir_data.state = IR_SPACE;
				else
					ir_data.state = IR_END;
			}
			else
			{
				ir_data.state = IR_FAIL;
			}
			break;
		case IR_FAIL:
			ir_data.decoded_val = 0;
			ir_data.bit_count = 0;
			ir_data.state = IR_IDLE;
			break;
		case IR_END:
			if((ir_data.time_lev > IR_T_SPACE_MIN) && (ir_data.time_lev < IR_T_SPACE_MAX))
			{
				ir_data.bit_count = 0;
				ir_data.decoded_flag = 1;
				ir_data.state = IR_IDLE;
			}
			else
			{
				ir_data.state = IR_FAIL;
			}
			break;
		}
	}
}

/* 
function with asm code to set data to ws2812
function to set the same colour for the entire array of leds
define set size of array
brightness array to use index in other functions
array of colours to write to leds
power on/off for the ws array using P-channel mosfet
*/


#include "main.h"

#define POWER_PIN P13
#define LED_POWER_ON 0
#define LED_POWER_OFF 1

#define LED_OUT_PIN		P14
#define LED_OUT_PIN_ASM	_P14

#define WS_LED_NUM 16//8

/*
8 leds * 3 bytes per led = 24
16 leds * 3 bytes per led = 48 bytes
*/
#define ARAY_SIZE (WS_LED_NUM*3)


/* 16-step brightness table: gamma = 2.1 */
const uint8_t gamma_table[WS_BRIGH_SIZE] = {
	10,
	15,
	23,
	31,
	41,
	53,
	66,
	81,
	97,
	115,
	134,
	155,
	178,
	202,
	228,
	255,
};

const ws_color_type ws_colors[WS_COL_SIZE]={
/*R G B*/
{255,0,0},
{255,50,0},
{255,100,0},
{255,150,0},
{255,200,0},
{0,255,0},
{0,255,50},
{0,255,100},
{0,255,150},
{0,255,200},
{0,0,255},
{100,0,235},
{150,0,235},
{200,0,235},
{255,0,235},
{255,240,240},
{0,0,0},
};

//ws2812
__data uint8_t led_array[ARAY_SIZE];

static void led_send_data();

void ws_power(uint8_t p)
{
	if(p==1)
	{
		POWER_PIN = LED_POWER_ON;
	}
	else
	{
		POWER_PIN = LED_POWER_OFF;
	}
}

void ws_oneColor(ws_data_type *c)
{
uint8_t i;
uint16_t u16green;
uint16_t u16red;
uint16_t u16blue;
u16green = ws_colors[c->colInd].green * gamma_table[c->brigInd] / 255;
u16red = ws_colors[c->colInd].red * gamma_table[c->brigInd] / 255;
u16blue = ws_colors[c->colInd].blue * gamma_table[c->brigInd] / 255;

	for(i = 0; i < ARAY_SIZE; i+=3)
	{
		led_array[i] = (uint8_t)u16green;
		led_array[i+1] = (uint8_t)u16red;
		led_array[i+2] = (uint8_t)u16blue;
	}
	led_send_data();
}
void ws_array(ws_colorIndex_type *arr, uint8_t bright_ind)
{
uint8_t i,j=0;
uint16_t u16green;
uint16_t u16red;
uint16_t u16blue;
	for(i = 0; i < ARAY_SIZE; i+=3)
	{
		u16green = ws_colors[arr[j]].green * gamma_table[bright_ind] / 255;
		u16red = ws_colors[arr[j]].red * gamma_table[bright_ind] / 255;
		u16blue = ws_colors[arr[j]].blue * gamma_table[bright_ind] / 255;
		j++;
		
		led_array[i] = (uint8_t)u16green;
		led_array[i+1] = (uint8_t)u16red;
		led_array[i+2] = (uint8_t)u16blue;
	}
	led_send_data();
}

/*
comment that explains the asm code for ws2812

note: high bit first
byte from array is read from local ram (the small one) indirectly by getting the address in r0.
r0 is incremented after each byte finishes sending.
Size or array is written in r7 and decremented after each byte using djnz (decrement and jump if not zero).
Pin gets value in 2 ways: setb/clr and mov.
for each bit in byte the byte is loaded in accumulator from r5. A is rotated left with carry. That gets the bit we want in the carry location.
mov has a variant that writes the carry bit to a bit addresable location, like the out port.
After the data byte from r5 gets in A and rotated, the result is put back in r5.
To keep track of how many times the byte is rotated: at the start of each byte r6 gets value 1; this gets rotated with carry after the bit sequence; if the 1 is not in carry yet then jnc will loop. If 1 is in carry then we did 8 bits. reset r6 and check for next byte.
After a byte ends the low pin state lasts longer than the long low. this is fine because the reset (to sent new data) is 50us of low. The long low is shorter than reset time ant this allows the fetching of next byte.
Also setb takes 4 cycles so that keeps the pin low.
*/
static void led_send_data()
{
/* write data from led_array in asm to pin */
__asm
	mov	r7,#ARAY_SIZE; array size
	mov	r6,#0x01; for counting bits

	mov	r0,#(_led_array); 
	mov a,@r0; indirect read byte

ready$:
	mov	r6,#0x01; for counting bits
	mov a,@r0; get next
	mov r5,a; put next in r5
start$:
	setb LED_OUT_PIN_ASM; T0 - pin high
	mov a,r5; T1 - get byte
	rlc a; T2 - rotate carry
	mov LED_OUT_PIN_ASM,C; T3 T4 T5 T6 - put carry to pin
	mov r5,a; T7 - save new - maybe put before pin write
	mov a,r6; T8 get bit counter
	rlc a; T9 increment bit counter
	clr LED_OUT_PIN_ASM; T10 T11 T12 T13 - pin low
	mov r6,a; T14 save counter
	jnc start$; T15
	inc r0; T16 - increment address
	djnz r7,ready$; T17 T18 T19 T20 dec and jump not zero		
__endasm;
}

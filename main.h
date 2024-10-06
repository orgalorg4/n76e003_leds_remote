/*
a whole mess of data types and function prototipes to be used in multiple files
*/

#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <n76e003.h>

//ws2812
#define WS_BRIGH_SIZE 12
//brightness increase/decrease
#define WS_BRIGH_PLUS (WS_BRIGH_SIZE + 1)
#define WS_BRIGH_MINUS (WS_BRIGH_SIZE + 2)

typedef enum{
WS_RED_0,
WS_RED_1,
WS_RED_2,
WS_RED_3,
WS_RED_4,
WS_GREEN_0,
WS_GREEN_1,
WS_GREEN_2,
WS_GREEN_3,
WS_GREEN_4,
WS_BLUE_0,
WS_BLUE_1,
WS_BLUE_2,
WS_BLUE_3,
WS_BLUE_4,
WS_WHITE,
WS_OFF,
WS_COL_SIZE,
WS_COL_NEXT,
WS_COL_PREV,
}ws_colorIndex_type;

extern const uint8_t gamma_table[WS_BRIGH_SIZE];

typedef struct{
uint8_t red;
uint8_t green;
uint8_t blue;
}ws_color_type;

typedef struct{
ws_colorIndex_type colInd;
uint8_t brigInd;
}ws_data_type;


//power
typedef enum{
PRE_ON,
ON,
PRE_OFF,
OFF,
}power_state_type;

//battery
typedef enum{
BATTERY_OK,
BATTERY_NOT_OK,
}battery_state_type;

//outmux
typedef enum{
OUTMUX_WS_BUILTIN,
OUTMUX_WS_USB,
OUTMUX_PWM_USB,
OUTMUX_NONE,
OUTMUX_SIZE,
OUTMUX_NEXT,
}outmux_out_type;


typedef struct{
outmux_out_type outmux_out;
ws_data_type ws;
uint8_t pwm;
}ilum_data_type;


void ws_power(uint8_t p);
void ws_oneColor(ws_data_type *c);
void ws_array(ws_colorIndex_type *arr, uint8_t bright_ind);


void ir_mainfunction(power_state_type *power);

void buttons_mainfunction(power_state_type *power);

void power_mangage_mainf(power_state_type *power);

void IR_ISR_IR(void) __interrupt(12);	//ir pin caputre
//void IR_ISR_PIN(void) __interrupt(7);	//pin interrupts

void battery_mainfuncion(battery_state_type *b);
void battery_showstate();

void eeprom_read_data(ilum_data_type *data);
void eeprom_write_data(ilum_data_type *data);

//outmux functions
void outmux_out_set(outmux_out_type out);
void outmux_set_bright(uint8_t br);
void outmux_set_ws_colour(ws_colorIndex_type col);
void outmux_mainfunction(power_state_type *power);
void outmux_init();
void outmux_pwm_init();

#endif

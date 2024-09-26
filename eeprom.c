/*
program memory used as eeprom.
a page at the end of memory holds the data.
write with the programing sequence.
read as const variable.
no protection in case program reaches that page.
*/

/*
__code __at addr - can be set over program space
page size = 128bytes
IAP can erase one page. maybe not needed to use hole page if erase is not used.
need IAP instuctions to write.
read as const array - no IAP
only 0 values can be written; to have 0->1 a page erase is nedded
*/

#include "main.h"

#define EEPROM_DISABLE 0	//1 = disable

#define PAGE_SIZE 128
/* 
set start address to value after program end.
.map file says address of program end
use multiple of 0x80 to use a page.

last page starts at 3E00
0x3000 is close to end on memory 
*/
#define NVM_START_ADR 0x1600//0x1400
/*
set size of ram array that holds data to be written to nvm
*/
//#define RAM_MIRROR_SIZE 30

/*index of data in nvm*/
enum
{
NVM_OUTPUT_SELECT,
NVM_WS_COLOR_INDEX,
NVM_WS_BRIGHTNESS_INDEX,
NVM_PWM_BRIGHTNESS_INDEX,
NVM_RAM_MIRROR_SIZE,
};

#if (EEPROM_DISABLE != 1)
__code __at NVM_START_ADR unsigned char Data_Flash[PAGE_SIZE]={0,WS_RED_4,0,0,};
/* ram mirror for nvm - xdata is external ram, the big one */
__xdata uint8_t data_ram_mirror[NVM_RAM_MIRROR_SIZE];
#endif

#if (EEPROM_DISABLE != 1)
void flash_write_all();
#endif

void eeprom_read_data(ilum_data_type *data)
{
#if (EEPROM_DISABLE != 1)
	data->outmux_out = Data_Flash[NVM_OUTPUT_SELECT];
	data->ws.colInd = Data_Flash[NVM_WS_COLOR_INDEX];
	data->ws.brigInd = Data_Flash[NVM_WS_BRIGHTNESS_INDEX];
	data->pwm = Data_Flash[NVM_PWM_BRIGHTNESS_INDEX];
#else
	data->outmux_out = OUTMUX_PWM_USB;
	data->ws.colInd = WS_WHITE;
	data->ws.brigInd = 0;
	data->pwm = 0;
#endif
}

void eeprom_write_data(ilum_data_type *data)
{
#if (EEPROM_DISABLE != 1)
	uint8_t i,flag = 0;
	/*copy new data in ram mirror*/
	data_ram_mirror[NVM_OUTPUT_SELECT] = data->outmux_out;
	data_ram_mirror[NVM_WS_COLOR_INDEX] = data->ws.colInd;
	data_ram_mirror[NVM_WS_BRIGHTNESS_INDEX] = data->ws.brigInd;
	data_ram_mirror[NVM_PWM_BRIGHTNESS_INDEX] = data->pwm;
	
	/* compare ram mirror to data in flash */
	for(i=0;i<NVM_RAM_MIRROR_SIZE;i++)
	{
		if(data_ram_mirror[i] != Data_Flash[i])
		{
			flag = 1;
		}
	}
	/* if any value is diferent - write all */
	if(flag == 1)
	{
		flash_write_all();
	}
#endif
}

#if (EEPROM_DISABLE != 1)
void flash_write_all()
{
	uint8_t uci;
	/*enable iap*/
	TA_UNPROTECT();
	CHPCON |= 0x01;
	TA_UNPROTECT();
	IAPUEN |= 0x01;
	/*erase page - set page start and data*/
	IAPCN = 0x22; /* datasheet uc page 227 */
	IAPAH = (uint8_t)(NVM_START_ADR >> 8);
	IAPAL = (uint8_t)(NVM_START_ADR & 0xFF);
	IAPFD = 0xFF;
	/* do it */
	TA_UNPROTECT();
	IAPTRG |= 0x01;
	
	for(uci = 0; uci < NVM_RAM_MIRROR_SIZE; uci++)
	{
		/*write a byte - set address and data*/
		IAPCN = 0x21; /* datasheet uc page 227 */
		IAPAH = (uint8_t)(NVM_START_ADR >> 8);
		IAPAL = ((uint8_t)(NVM_START_ADR & 0xFF) + uci);
		IAPFD = data_ram_mirror[uci];
		/* do it */
		TA_UNPROTECT();
		IAPTRG |= 0x01;
	}

	/*disable iap*/
	TA_UNPROTECT();
	IAPUEN &= ~0x01;
	TA_UNPROTECT();
	CHPCON &= ~0x01;
}
#endif

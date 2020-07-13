/**
 * ssd1306.h
 */

#pragma	once

#include	"hal_i2c.h"
#include	<stdint.h>

// ##################################### BUILD definitions #########################################


// ##################################### MACRO definitions #########################################

#define	SSD1306_ADDR_PRI						0x3C
#define	SSD1306_ADDR_SEC						0x3D

// ####################################### structures ##############################################

typedef struct __attribute__((packed)) ssd1306_s {
	i2c_dev_info_t *psI2C ;								// size = 4
	epid_t		epid ;									// size = 4
	uint8_t		mem_mode ;
	uint8_t		segment ;								// pixel horizontal
	uint8_t		max_seg ;
	uint8_t		page ;
	uint8_t		max_page ;
	uint8_t		status ;
} ssd1306_t ;
DUMB_STATIC_ASSERT(sizeof(ssd1306_t) == 14) ;

// ######################################## public variables #######################################


// ################################### EXTERNAL FUNCTIONS ##########################################

void	ssd1306SetContrast(uint8_t Contrast) ;
void	ssd1306SetDisplayState(uint8_t State) ;
void	ssd1306SetTextCursor(uint8_t X, uint8_t Y) ;
int		ssd1306PutChar(int cChr) ;
void	ssd1306PutString(const char *) ;

int32_t ssd1306Identify(i2c_dev_info_t * psI2C_DI) ;
int32_t ssd1306Config(i2c_dev_info_t * psI2C_DI) ;
int32_t ssd1306Diagnostics(i2c_dev_info_t * psI2C_DI) ;

void	ssd1306Report(void) ;

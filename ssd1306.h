/**
 * ssd1306.h
 */

#pragma	once

#include	"hal_i2c.h"

// ##################################### MACRO definitions #########################################

#define	SSD1306_ADDR_PRI						0x3C
#define	SSD1306_ADDR_SEC						0x3D

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

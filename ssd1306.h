/**
 * ssd1306.h
 */

#pragma	once

#include	<stdint.h>

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

int32_t ssd1306Diagnostics(void) ;
int32_t ssd1306Identify(uint8_t eChan, uint8_t addr) ;
void	ssd1306Report(void) ;

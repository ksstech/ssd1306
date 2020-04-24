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

//uint8_t	ssd1306GetStatus(void) ;
//void	ssd1306SetScrollState(uint8_t State) ;
//void	ssd1306SetScanDirection(uint8_t State) ;
//void	ssd1306SetMemoryMode(uint8_t Mode) ;
//void	ssd1306SetOffset(uint8_t Offset) ;
//void	ssd1306SetInverse(uint8_t State) ;
//void	ssd1306SetSegmentAddr(uint8_t Segment) ;
//void	ssd1306SetPageAddr(uint8_t Page) ;
//void	ssd1306SetTextCursor(uint8_t X, uint8_t Y) ;
//void 	ssd1306Clear(void) ;
//void	ssd1306Init(void) ;

void	ssd1306SetContrast(uint8_t Contrast) ;
void	ssd1306SetDisplayState(uint8_t State) ;
void	ssd1306SetTextCursor(uint8_t X, uint8_t Y) ;
int		ssd1306PutChar(int cChr) ;
void	ssd1306PutString(const char *) ;

int32_t ssd1306Diagnostics(void) ;
int32_t ssd1306Identify(uint8_t eChan, uint8_t addr) ;
void	ssd1306Report(void) ;

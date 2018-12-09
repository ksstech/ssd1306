/*
 * ssd1306.h
 */

#pragma	once

#include	"hal_i2c.h"
#include	<stdint.h>

// ####################################### LCD definitions #########################################

#define	LCD_TYPE_64_48						1			// Wemos D1 Mini OLED shield
#define	LCD_TYPE_128_64						2
#define	LCD_TYPE							LCD_TYPE_64_48

#if		(LCD_TYPE == LCD_TYPE_64_48)
	#define	LCD_WIDTH						64			// pixels horizontal
	#define	LCD_HEIGHT						48			// pixels vertical
	#define	LCD_COLUMNS						10			// character columns 5x7 font size
	#define	LCD_LINES						6			// characters rows(lines)
#elif	(LCD_TYPE == LCD_TYPE_64_48)
	#define	LCD_WIDTH						64		// pixels horizontal
	#define	LCD_HEIGHT						48		// pixels vertical
	#define	FONT_WIDTH						6
	#define	FONT_HEIGHT						8
	#define	LCD_COLUMNS						10		// character columns 5x7 font size
	#define	LCD_LINES						6		// characters rows(lines)
#else
	#error "No/invalid LCD Type selected!!!"
#endif

// ###################################### FONT definitions #########################################

#define	FONT_5X7							1			// smallest possible font
#define	FONT_7X7							2			// normal 1:1 aspect font
#define	FONT_14X14							3			// double 1:1 aspect font
#define	FONT_22X22							4			// triple 1:1 aspect font
#define	FONT_TYPE							FONT_5X7

#if		(FONT_TYPE == FONT_5X7)
	#define	FONT_WIDTH						6
	#define	FONT_HEIGHT						8
#elif	(FONT_TYPE == FONT_7X7)
	#define	FONT_WIDTH						8
	#define	FONT_HEIGHT						8
#elif	(FONT_TYPE == FONT_14X714)
	#define	FONT_WIDTH						16
	#define	FONT_HEIGHT						16
#elif	(FONT_TYPE == FONT_22X22)
	#define	FONT_WIDTH						24
	#define	FONT_HEIGHT						24
#else
	#error "No/invalid Font Size selected!!!"
#endif

// ###################################### Anomaly handling #########################################

#if		(LCD_TYPE == LCD_TYPE_64_48) && (FONT_TYPE == FONT_5X7)
	#define	ANOMALY_OFFSET					32			// WEMOS Mini D1 OLED shield quirk
	#define	LCD_SPARE_PIXELS				(LCD_WIDTH - (LCD_COLUMNS * FONT_WIDTH))
#else
	#define	SSD1306_OFFSET					0
	#define	LCD_SPARE_PIXELS				0
#endif

// ##################################### MACRO definitions #########################################

// Absolute maximum SSD1306 specs
#define	SSD1306_MAX_SEGMENT					128
#define	SSD1306_MAX_COLUMN					64
#define	SSD1306_MAX_PAGE					8

// Scrolling commands
#define ssd1306RIGHT_HORIZONTAL_SCROLL				0x26
#define ssd1306LEFT_HORIZONTAL_SCROLL				0x27
#define ssd1306VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29
#define ssd1306VERTICAL_AND_LEFT_HORIZONTAL_SCROLL	0x2A
#define ssd1306DEACTIVATE_SCROLL					0x2E
#define ssd1306ACTIVATE_SCROLL						0x2F
#define ssd1306SET_VERTICAL_SCROLL_AREA 			0xA3

#define ssd1306SETCONTRAST							0x81		// 0 -> 255

#define ssd1306DISPLAYALLON_RESUME					0xA4
#define ssd1306DISPLAYALLON							0xA5

#define ssd1306NORMALDISPLAY						0xA6
#define ssd1306INVERTDISPLAY						0xA7

#define ssd1306DISPLAYOFF							0xAE
#define ssd1306DISPLAYON							0xAF

#define ssd1306SETDISPLAYOFFSET						0xD3
#define ssd1306SETCOMPINS							0xDA

#define ssd1306SETVCOMDESELECT						0xDB
#define ssd1306SETVCOMDESELECT_0v65					0x00
#define ssd1306SETVCOMDESELECT_0v77					0x20
#define ssd1306SETVCOMDESELECT_0v83					0x30

#define ssd1306SETDISPLAYCLOCKDIV					0xD5
#define ssd1306SETPRECHARGE							0xD9		// P2=10->F0 | P1=01->0F

#define ssd1306SETMULTIPLEX							0xA8

#define ssd1306SETLOWCOLUMN							0x00		// set lower column start for page addressing (up to 0x0F)
#define ssd1306SETHIGHCOLUMN						0x10		// set higher column start for page addressing (up to 0x1F)
#define ssd1306MEMORYMODE							0x20
#define ssd1306COLUMNADDR							0x21
#define ssd1306PAGEADDR								0x22		// +0->7(start, 0=reset)), +0->7(end, 7=reset)
#define ssd1306PAGESTART							0xB0		// to 0xB7

#define ssd1306SETSTARTLINE							0x40		// to 0x4F, set display start line

#define ssd1306COMSCANINC							0xC0
#define ssd1306COMSCANDEC							0xC8

#define ssd1306SEGREMAP								0xA0

#define ssd1306CHARGEPUMP							0x8D
#define ssd1306CHARGEPUMP_OFF						0x10
#define ssd1306CHARGEPUMP_ON						0x14


#define ssd1306EXTERNALVCC							0x01
#define ssd1306SWITCHCAPVCC							0x02

// ###################################### BUILD : CONFIG definitions ###############################

#define	ssd1306VERSION								"v0.0.1.1"
#define	SSD1306_ADDR_PRI							0x3C
#define	SSD1306_ADDR_SEC							0x3D

// ############################## BUILD : FreeRTOS Task definitions ################################


// ####################################### structures ##############################################

typedef struct _ssd1306 {
	halI2Cdev_t	sI2Cdev ;
	uint8_t		mem_mode ;
	uint8_t		segment ;								// pixel horizontal
	uint8_t		max_seg ;
	uint8_t		page ;
	uint8_t		max_page ;
} ssd1306_t ;

// ######################################## public variables #######################################

extern	const uint8_t	font[] ;
extern	ssd1306_t sSSD1306 ;

// ################################### EXTERNAL FUNCTIONS ##########################################

uint8_t	ssd1306GetStatus(ssd1306_t * pDev) ;
void	ssd1306SetDisplayState(ssd1306_t * pDev, uint8_t State) ;
void	ssd1306SetScrollState(ssd1306_t * pDev, uint8_t State) ;
void	ssd1306SetScanDirection(ssd1306_t * pDev, uint8_t State) ;
void	ssd1306SetMemoryMode(ssd1306_t * pDev, uint8_t Mode) ;
void	ssd1306SetOffset(ssd1306_t * pDev, uint8_t Offset) ;
void	ssd1306SetContrast(ssd1306_t * pDev, uint8_t Contrast) ;
void	ssd1306SetInverse(ssd1306_t * pDev, uint8_t State) ;
void	ssd1306SetSegmentAddr(ssd1306_t * pDev, uint8_t Segment) ;
void	ssd1306SetPageAddr(ssd1306_t * pDev, uint8_t Page) ;
void	ssd1306SetTextCursor(ssd1306_t * pDev, uint8_t X, uint8_t Y) ;
void 	ssd1306Clear(ssd1306_t * pDev) ;
void	ssd1306Init(ssd1306_t *) ;
int		ssd1306PutChar(ssd1306_t *, uint8_t) ;
int		ssd1306PutC(int) ;
void	ssd1306PutString(ssd1306_t *, uint8_t *) ;

int32_t ssd1306Diagnostics(ssd1306_t * pDev) ;
int32_t ssd1306Identify(uint8_t eChan, uint8_t addr) ;
void	ssd1306Report(int32_t Handle, ssd1306_t * pDev) ;

/**
 * ssd1306.c
 * https://github.com/wtfuzz/ssd1306_text
 *
 *	Date		Ver			Comments/changes
 * 20180630		0.0.1.0		Initial version
 * 20181129		0.0.1.1		Enabled fade-in support
 * 20190105		0.0.1.2		Simplified API to support single SSD1306 device
 * 20200413		0.0.1.3		Uncoupled from fonts to facilitate ILI9341/ST7789V support
 * 							Removed duplicate definitions
 */

#include	"x_syslog.h"
#include	"x_errors_events.h"
#include	"x_definitions.h"
#include	"x_systiming.h"

#include	"hal_debug.h"
#include	"hal_i2c.h"

#include	"ssd1306.h"
#include	"fonts/fonts.h"

#include	<string.h>

#define	debugFLAG					0x0000

#define	debugCMDS					(debugFLAG & 0x0001)
#define	debugTIMING					(debugFLAG & 0x0004)
#define	debugCONTRAST				(debugFLAG & 0x0008)

#define	debugPARAM					(debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG & 0x8000)

// ###################################### BUILD : CONFIG definitions ###############################

#define	ssd1306VERSION							"v0.0.1.3"

#define	ssd1306FONT								FONT5X7
#define	ssd1306FONT_HEIGHT						CONCAT2(ssd1306FONT, _HEIGHT)
#define	ssd1306FONT_WIDTH						CONCAT2(ssd1306FONT, _WIDTH)

// ####################################### LCD definitions #########################################

#define	LCD_TYPE_64_48							1		// Wemos D1 Mini OLED shield
#define	LCD_TYPE_128_64							2
#define	LCD_TYPE								LCD_TYPE_64_48

#if		(LCD_TYPE == LCD_TYPE_64_48)
	#define	LCD_WIDTH							64		// pixels horizontal
	#define	LCD_HEIGHT							48		// pixels vertical
#elif	(LCD_TYPE == LCD_TYPE_128_64)
	#define	LCD_WIDTH							128		// pixels horizontal
	#define	LCD_HEIGHT							64		// pixels vertical
#else
	#error "No/invalid LCD Type selected!!!"
#endif

#define	LCD_COLUMNS								(LCD_WIDTH / ssd1306FONT_WIDTH)
#define	LCD_LINES								(LCD_HEIGHT / ssd1306FONT_HEIGHT)
#define	LCD_SPARE_PIXELS						(LCD_WIDTH - (LCD_COLUMNS * ssd1306FONT_WIDTH))

#define	ANOMALY_OFFSET							32		// WEMOS Mini D1 OLED shield quirk

// ##################################### MACRO definitions #########################################

// Absolute maximum SSD1306 specs
#define	SSD1306_MAX_SEGMENT						128
#define	SSD1306_MAX_COLUMN						64
#define	SSD1306_MAX_PAGE						8

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
// ####################################### structures ##############################################

typedef struct _ssd1306 {
	halI2Cdev_t	sI2Cdev ;
	uint8_t		mem_mode ;
	uint8_t		segment ;								// pixel horizontal
	uint8_t		max_seg ;
	uint8_t		page ;
	uint8_t		max_page ;
} ssd1306_t ;

// ################################ private static variables ######################################

static ssd1306_t sSSD1306 = { .max_seg=LCD_WIDTH, .max_page=LCD_HEIGHT / ssd1306FONT_HEIGHT } ;

// ################################# private public variables #####################################


/**
 * ssd1306SendCommand_?() - send a single/dual/triple byte command(s) to the controller
 * @param	Cmd? - command(s) to send
 */
void	ssd1306SendCommand_1(uint8_t Cmd1) {
	uint8_t cBuf[2] ;
	cBuf[0]	= 0x00 ;									// command following
	cBuf[1]	= Cmd1 ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;
}

void	ssd1306SendCommand_2(uint8_t Cmd1, uint8_t Cmd2) {
	uint8_t cBuf[3] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;
}

void	ssd1306SendCommand_3(uint8_t Cmd1, uint8_t Cmd2, uint8_t Cmd3) {
	uint8_t cBuf[4] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	cBuf[3]	= Cmd3 ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;
}

uint8_t	ssd1306GetStatus() {
	uint8_t ssd1306Status ;
	halI2C_Read(&sSSD1306.sI2Cdev, &ssd1306Status, sizeof(ssd1306Status)) ;
	return ssd1306Status ;
}

/* Functions to configure the basic operation of the controller */

void	ssd1306SetDisplayState(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306DISPLAYON : ssd1306DISPLAYOFF) ;
}

void	ssd1306SetScrollState(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306ACTIVATE_SCROLL : ssd1306DEACTIVATE_SCROLL) ;
}

void	ssd1306SetScanDirection(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306COMSCANINC : ssd1306COMSCANDEC) ;
}

/**
 *  @param	Mode = 0/horizontal  1/vertical  2/page
 */
void	ssd1306SetMemoryMode(uint8_t Mode) {
	IF_myASSERT(debugPARAM, Mode < 0x03) ;
	sSSD1306.mem_mode	= Mode ;
	ssd1306SendCommand_2(ssd1306MEMORYMODE, Mode) ;
}

/**
 * @param	Offset - 0->63 Pan up or down
 */
void	ssd1306SetOffset(uint8_t Offset) {
	IF_myASSERT(debugPARAM, Offset < SSD1306_MAX_COLUMN) ;
	ssd1306SendCommand_2(ssd1306SETDISPLAYOFFSET, Offset) ;
}

/**
 * @param	Contrast - 0->255 dim to bright
 */
void	ssd1306SetContrast(uint8_t Contrast) {
/* See https://github.com/ThingPulse/esp8266-oled-ssd1306/issues/134
 * Contrast		PC_0 	PC_1	Vcom
 * 0x00			0x1F	0x11	0x00
 * 0x10			0x1E	0x11
 * 0x20			0x2D	0x22
 * 0x30			0x3C	0x33
 * 0x40			0x4B	0x44
 * 0x50			0x5A	0x55	0x00
 * 0x60			0x69	0x66	0x20
 * 0x70			0x78	0x77
 * 0x80			0x87	0x88
 * 0x90			0x96	0x99
 * 0xA0			0xA5	0xAA	0x20
 * 0xB0			0xB4	0xBB	0x30
 * 0xC0			0xC3	0xCC
 * 0xD0			0xD2	0xDD
 * 0xE0			0xE1	0xEE
 * 0xFF			0xF1 	0xFF	0x30 */
	uint8_t RelContrast = Contrast >> 4 ;
#if 	0
	uint8_t PreCharge = RelContrast == 0x00 ? 0x10 : RelContrast << 4 ;
	PreCharge |= RelContrast  == 0x0F ? 0x01 : 0x0F - RelContrast ;
#elif	1
	uint8_t PreCharge = RelContrast << 4 | RelContrast ;
	PreCharge = PreCharge == 0x00 ? 0x11 : PreCharge ;
#endif
	ssd1306SendCommand_2(ssd1306SETPRECHARGE, PreCharge) ;
	ssd1306SendCommand_2(ssd1306SETCONTRAST, Contrast) ;
	uint8_t NewVcom = RelContrast < 0x06 ? 0x00 : RelContrast < 0x0B ? 0x20 : 0x30 ;
	ssd1306SendCommand_2(ssd1306SETVCOMDESELECT, NewVcom) ;

	if (Contrast == 0) {
		ssd1306SetDisplayState(0) ;		// switch display off
	} else {
		ssd1306SetDisplayState(1) ;		// switch display on
	}
	IF_PRINT(debugCONTRAST, "Contrast=0x%02X  PreCharge=0x%02X  Vcom=0x%02X\n", Contrast, PreCharge, NewVcom) ;
}

void	ssd1306SetInverse(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306INVERTDISPLAY : ssd1306NORMALDISPLAY) ;
}

/* Functions to set the display graphics/pixel cursor and page position */

void	ssd1306SetSegmentAddr(uint8_t Segment) {
	sSSD1306.segment = (Segment < sSSD1306.max_seg) ? Segment : 0 ;
	ssd1306SendCommand_3(ssd1306COLUMNADDR, Segment + ANOMALY_OFFSET, sSSD1306.max_seg + ANOMALY_OFFSET) ;
}

void	ssd1306SetPageAddr(uint8_t Page) {
	sSSD1306.page = Page < sSSD1306.max_page ? Page : 0 ;
	ssd1306SendCommand_3(ssd1306PAGEADDR, Page, sSSD1306.max_page) ;
}

/* High level functions to control window/cursor/scrolling etc */

void	ssd1306SetTextCursor(uint8_t X, uint8_t Y) { ssd1306SetPageAddr(Y) ; ssd1306SetSegmentAddr(X * ssd1306FONT_WIDTH) ; }

void 	ssd1306Clear(void) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, systimerSSD1306) ;
	#define LINES	3									// 1=14,129  2=12,033  3=11,338  6=10,645 uSec
	uint8_t	cBuf[1+LCD_WIDTH*LINES] ;
	memset(cBuf, 0, sizeof(cBuf)) ;
	cBuf[0]	= 0x40 ;									// writing data
	for(int32_t i = 0; i < LCD_LINES; i += LINES) {
		ssd1306SetTextCursor(0, i) ;
		halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;
	}
	ssd1306SetTextCursor(0, 0) ;
	IF_EXEC_1(debugTIMING, xSysTimerStop, systimerSSD1306) ;
}

/**
 * ssd1306Init() - initialise the controller
 */
void	ssd1306Init(void) {
	IF_SYSTIMER_INIT(debugTIMING, systimerSSD1306, systimerCLOCKS, "SSD1306", myMS_TO_CLOCKS(2), myMS_TO_CLOCKS(15)) ;
	ssd1306SendCommand_2(ssd1306SETMULTIPLEX, LCD_HEIGHT-1) ;
	ssd1306SetOffset(0) ;
	ssd1306SendCommand_1(ssd1306SETSTARTLINE | 0x0) ;					// ok
	ssd1306SendCommand_1(ssd1306SEGREMAP | 0x1) ;						// ok
	ssd1306SendCommand_1(ssd1306COMSCANDEC) ;							// ok
	ssd1306SendCommand_2(ssd1306SETCOMPINS, 0x12) ;						// ok

//	ssd1306SetContrast(0x00) ;
	ssd1306SendCommand_2(ssd1306SETDISPLAYCLOCKDIV, 0x80) ;				// ok

//	ssd1306SendCommand_2(ssd1306SETPRECHARGE, 0xF1) ;					// ok, internal Vcc
//	ssd1306SendCommand_2(ssd1306SETVCOMDESELECT, 0x40) ;				// ok
	ssd1306SendCommand_2(ssd1306CHARGEPUMP, ssd1306CHARGEPUMP_ON) ;		// ok, internal Vcc

	ssd1306SetMemoryMode(0x00) ;										// horizontal mode, wrap to next page & around

	ssd1306SendCommand_1(ssd1306DISPLAYALLON_RESUME) ;					// ok
	ssd1306SetInverse(0) ;
	ssd1306SetScrollState(0) ;
	ssd1306SetDisplayState(1) ;

	ssd1306Clear() ;
}

/**
 * @brief Write a single character at the current cursor position
 *
 * Using vertical addressing mode, we can have the hardware
 * automatically move to the next column after writing 7 pixels
 * in each page (rows).
 */
int		ssd1306PutChar(int cChr) {
	if ((sSSD1306.sI2Cdev.epidI2C.devclass != devSSD1306) || (sSSD1306.sI2Cdev.epidI2C.subclass != subDSP64X48)) {
		return cChr ;
	}
	IF_EXEC_1(debugTIMING, xSysTimerStart, systimerSSD1306_2) ;
	const char * pFont = &font5X7[cChr * (ssd1306FONT_WIDTH - 1)] ;
	uint8_t	cBuf[ssd1306FONT_WIDTH + 1 ] ;
	int32_t	i ;
	IF_PRINT(debugCMDS,"%c : %02x-%02x-%02x-%02x-%02x\n", cChr, *pFont, *(pFont+1), *(pFont+2), *(pFont+3), *(pFont+4)) ;

	cBuf[0]	= 0x40 ;											// data following
	for(i = 1; i < ssd1306FONT_WIDTH; cBuf[i++] = *pFont++) ;	// copy font bitmap across
	cBuf[i]	= 0x00 ;											// do the blank separating segment
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;		// send the character

	// update the cursor location
	sSSD1306.segment	+= ssd1306FONT_WIDTH ;
	if (sSSD1306.segment >= (sSSD1306.max_seg - LCD_SPARE_PIXELS)) {
		++sSSD1306.page ;
		if (sSSD1306.page == sSSD1306.max_page) {
			sSSD1306.page = 0 ;
		}
		ssd1306SetPageAddr(sSSD1306.page) ;
		ssd1306SetSegmentAddr(0) ;
	}
	IF_EXEC_1(debugTIMING, xSysTimerStop, systimerSSD1306_2) ;
	return cChr ;
}

void	ssd1306PutString(const char * pString) { while(*pString) ssd1306PutChar(*pString++) ; }

int32_t ssd1306Diagnostics() {
	SL_DBG("ssd1306: Filling screen\n") ;
	ssd1306SetTextCursor(0, 0) ; ssd1306PutString("|00000000|") ;
	ssd1306SetTextCursor(0, 1) ; ssd1306PutString("+11111111+") ;
	ssd1306SetTextCursor(0, 2) ; ssd1306PutString("=22222222=") ;
	ssd1306SetTextCursor(0, 3) ; ssd1306PutString("[33333333]") ;
	ssd1306SetTextCursor(0, 4) ; ssd1306PutString("{44444444}") ;
	ssd1306SetTextCursor(0, 5) ; ssd1306PutString("(55555555)") ;

	SL_DBG("ssd1306: Writing bars\n") ;
	uint8_t cBuf[1+LCD_WIDTH] ;
	ssd1306SetTextCursor(0, 0) ;
	cBuf[0]	= 0x40 ;								// sending data
	memset(&cBuf[1], 0x88, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xCC, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xEE, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x77, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x33, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x11, LCD_WIDTH) ;
	halI2C_Write(&sSSD1306.sI2Cdev, cBuf, sizeof(cBuf)) ;

	SL_DBG("ssd1306: Clearing the screen\n") ;
	ssd1306Clear() ;
	ssd1306SetPageAddr(0) ;
	ssd1306SetSegmentAddr(0) ;
	for(int32_t i = 0; i < (LCD_COLUMNS * LCD_LINES); i++) ssd1306PutChar(i + CHR_SPACE) ;
	return erSUCCESS ;
}

int32_t	ssd1306Identify(uint8_t eChan, uint8_t Addr) {
	sSSD1306.sI2Cdev.chanI2C	= eChan ;
	sSSD1306.sI2Cdev.addrI2C	= Addr ;
	sSSD1306.sI2Cdev.dlayI2C	= pdMS_TO_TICKS(100) ;

	// detect & verify existence.
	uint8_t ssd1306Status = ssd1306GetStatus();
	IF_PRINT(debugCMDS, "ssd1306 Chan=%d Addr=%d Status = 0x%02x\n", eChan, Addr, ssd1306Status) ;

	// valid device signature not found
	if ((ssd1306Status & 0x03) != 0x03) {
		sSSD1306.sI2Cdev.chanI2C = sSSD1306.sI2Cdev.addrI2C	= sSSD1306.sI2Cdev.dlayI2C	= 0 ;
		return erFAILURE ;
	}

	// valid device signature found
	sSSD1306.sI2Cdev.epidI2C.devclass	= devSSD1306 ;
	sSSD1306.sI2Cdev.epidI2C.subclass	= subDSP64X48 ;
	sSSD1306.sI2Cdev.epidI2C.epuri		= URI_UNKNOWN ;
	sSSD1306.sI2Cdev.epidI2C.epunit	= UNIT_UNKNOWN ;
	ssd1306Init() ;
	return erSUCCESS ;
}

void	ssd1306Report(void) {
	PRINT("SSD1306: segment:%d max_seg:%d page:%d max_page:%d\n",
			sSSD1306.segment, sSSD1306.max_seg, sSSD1306.page, sSSD1306.max_page) ;
}

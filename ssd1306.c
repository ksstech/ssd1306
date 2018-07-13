/**
 * ssd1306.c
 * https://github.com/wtfuzz/ssd1306_text
 */

#include	"ssd1306.h"
#include	"x_errors_events.h"
#include	"x_definitions.h"
#include	"x_ticktimer.h"

// ######################################## DEBUG MACROS ###########################################

#define	ssd1306DEBUG				0x0002

#define	ssd1306DEBUG_CMDS			(ssd1306DEBUG & 0x0001)
#define	ssd1306DEBUG_PARAM			(ssd1306DEBUG & 0x0002)
#define	ssd1306DEBUG_TIMING			(ssd1306DEBUG & 0x0004)

// ################################ private static variables ######################################


// ################################# private public variables #####################################

ssd1306_t sSSD1306 = {
		.max_seg	= LCD_WIDTH,
		.max_page	= LCD_HEIGHT / FONT_HEIGHT,
} ;

/**
 * ssd1306SendCommand_?() - send a single/dual/triple byte command(s) to the controller
 * @param	pDev - pointer to device control structure
 * @param	Cmd1 - command to send
 * @param	Cmd2 - optional parameter 1 to send
 * @param	Cmd3 - optional parameter 2 to send
 */
void	ssd1306SendCommand_1(ssd1306_t * pDev, uint8_t Cmd1) {
	uint8_t cBuf[2] ;
	cBuf[0]	= 0x00 ;									// command following
	cBuf[1]	= Cmd1 ;
	halI2C_Write(&pDev->sI2Cdev, &cBuf[0], 2) ;
}

void	ssd1306SendCommand_2(ssd1306_t * pDev, uint8_t Cmd1, uint8_t Cmd2) {
	uint8_t cBuf[3] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	halI2C_Write(&pDev->sI2Cdev, &cBuf[0], 3) ;
}

void	ssd1306SendCommand_3(ssd1306_t * pDev, uint8_t Cmd1, uint8_t Cmd2, uint8_t Cmd3) {
	uint8_t cBuf[4] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	cBuf[3]	= Cmd3 ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, 4) ;
}

uint8_t	ssd1306GetStatus(ssd1306_t * pDev) {
	uint8_t ssd1306Status ;
	halI2C_Read(&pDev->sI2Cdev, &ssd1306Status, 1) ;
	return ssd1306Status ;
}

/* Functions to configure the basic operation of the controller */

void	ssd1306SetDisplayState(ssd1306_t * pDev, uint8_t State) {
	ssd1306SendCommand_1(pDev, State ? ssd1306DISPLAYON : ssd1306DISPLAYOFF) ;
}

void	ssd1306SetScrollState(ssd1306_t * pDev, uint8_t State) {
	ssd1306SendCommand_1(pDev, State ? ssd1306ACTIVATE_SCROLL : ssd1306DEACTIVATE_SCROLL) ;
}

void	ssd1306SetScanDirection(ssd1306_t * pDev, uint8_t State) {
	ssd1306SendCommand_1(pDev, State ? ssd1306COMSCANINC : ssd1306COMSCANDEC) ;
}

/**
 *  @param	Mode = 0/horizontal  1/vertical  2/page
 */
void	ssd1306SetMemoryMode(ssd1306_t * pDev, uint8_t Mode) {
	IF_myASSERT(ssd1306DEBUG_PARAM, Mode < 0x03) ;
	pDev->mem_mode	= Mode ;
	ssd1306SendCommand_2(pDev, ssd1306MEMORYMODE, Mode) ;
}

/**
 * @param	Offset - 0->63 Pan up or down
 */
void	ssd1306SetOffset(ssd1306_t * pDev, uint8_t Offset) {
	IF_myASSERT(ssd1306DEBUG_PARAM, Offset < SSD1306_MAX_COLUMN) ;
	ssd1306SendCommand_2(pDev, ssd1306SETDISPLAYOFFSET, Offset) ;
}

/**
 * @param	Contrast - 0->255 dim to bright
 */
void	ssd1306SetContrast(ssd1306_t * pDev, uint8_t Contrast) {
	ssd1306SendCommand_2(pDev, ssd1306SETCONTRAST, Contrast) ;
}

void	ssd1306SetInverse(ssd1306_t * pDev, uint8_t State) {
	ssd1306SendCommand_1(pDev, State ? ssd1306INVERTDISPLAY : ssd1306NORMALDISPLAY) ;
}

/* Functions to set the display graphics/pixel cursor and page position */

void	ssd1306SetSegmentAddr(ssd1306_t * pDev, uint8_t Segment) {
	pDev->segment	= (Segment < pDev->max_seg) ? Segment : 0 ;
	ssd1306SendCommand_3(pDev, ssd1306COLUMNADDR, Segment + ANOMALY_OFFSET, pDev->max_seg + ANOMALY_OFFSET) ;
}

void	ssd1306SetPageAddr(ssd1306_t * pDev, uint8_t Page) {
	pDev->page	= Page < pDev->max_page ? Page : 0 ;
	ssd1306SendCommand_3(pDev, ssd1306PAGEADDR, Page, pDev->max_page) ;
}

/* High level functions to control window/corsor/scrolling etc */

void	ssd1306SetTextCursor(ssd1306_t * pDev, uint8_t X, uint8_t Y) {
	ssd1306SetPageAddr(pDev, Y) ;
	ssd1306SetSegmentAddr(pDev, X * FONT_WIDTH) ;
}

void 	ssd1306Clear(ssd1306_t * pDev) {		// 1=14,129  2=12,033  3=11,338  6=10,645 uSec
	IF_EXEC_1(ssd1306DEBUG_TIMING, xClockTimerStart, clockTIMER_SSD1306) ;
	#define LINES	3
	uint8_t	cBuf[1+LCD_WIDTH*LINES] ;
	memset(cBuf, 0, sizeof(cBuf)) ;
	cBuf[0]	= 0x40 ;									// writing data
	for(int32_t i = 0; i < LCD_LINES; i += LINES) {
		ssd1306SetTextCursor(pDev, 0, i) ;
		halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;
	}
	ssd1306SetTextCursor(pDev, 0, 0) ;
	IF_EXEC_1(ssd1306DEBUG_TIMING, xClockTimerStop, clockTIMER_SSD1306) ;
}

/**
 * ssd1306Init() - initialise the controller
 * @param	pDev - pointer to the device control structure
 */
void	ssd1306Init(ssd1306_t * pDev) {
	ssd1306SendCommand_2(pDev, ssd1306SETMULTIPLEX, LCD_HEIGHT-1) ;
	ssd1306SetOffset(pDev, 0) ;
	ssd1306SendCommand_1(pDev, ssd1306SETSTARTLINE | 0x0) ;					// ok
	ssd1306SendCommand_1(pDev, ssd1306SEGREMAP | 0x1) ;						// ok
	ssd1306SendCommand_1(pDev, ssd1306COMSCANDEC) ;							// ok
	ssd1306SendCommand_2(pDev, ssd1306SETCOMPINS, 0x12) ;					// ok

	ssd1306SetContrast(pDev, 0x8F) ;
	ssd1306SendCommand_2(pDev, ssd1306SETDISPLAYCLOCKDIV, 0x80) ;			// ok

	ssd1306SendCommand_2(pDev, ssd1306SETPRECHARGE, 0xF1) ;					// ok, internal Vcc
	ssd1306SendCommand_2(pDev, ssd1306SETVCOMDESELECT, 0x40) ;				// ok
	ssd1306SendCommand_2(pDev, ssd1306CHARGEPUMP, 0x14) ;					// ok, internal Vcc

	ssd1306SetMemoryMode(pDev, 0x00) ;										// horizontal mode, wrap to next page & around

	ssd1306SendCommand_1(pDev, ssd1306DISPLAYALLON_RESUME) ;				// ok
	ssd1306SetInverse(pDev, 0) ;
	ssd1306SetScrollState(pDev, 0) ;
	ssd1306SetDisplayState(pDev, 1) ;

	ssd1306Clear(pDev) ;
}

/**
 * @brief Write a single character at the current cursor position
 *
 * Using vertical addressing mode, we can have the hardware
 * automatically move to the next column after writing 7 pixels
 * in each page (rows).
 */
int		ssd1306PutChar(ssd1306_t * pDev, uint8_t cChr) {
	if ((pDev->sI2Cdev.epidI2C.devclass != devSSD1306) || (pDev->sI2Cdev.epidI2C.subclass != subDSP64X48)) {
		return cChr ;
	}
	IF_EXEC_1(ssd1306DEBUG_TIMING, xClockTimerStart, clockTIMER_SSD1306_2) ;
	uint8_t * pFont = (uint8_t *) &font[cChr * (FONT_WIDTH-1)] ;
	uint8_t	cBuf[FONT_WIDTH+1] ;
	int32_t	i ;
	IF_PRINT(ssd1306DEBUG_CMDS,"%c : %02x-%02x-%02x-%02x-%02x\n", cChr, *pFont, *(pFont+1), *(pFont+2), *(pFont+3), *(pFont+4)) ;

	cBuf[0]	= 0x40 ;									// data following
	for(i = 1; i < FONT_WIDTH; i++) {
		cBuf[i]	= *pFont++ ;							// copy font bitmap across
	}
	cBuf[i]	= 0x00 ;									// do the blank separating segment
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;	// send the character

	// update the cursor location
	pDev->segment	+= FONT_WIDTH ;
	if (pDev->segment >= (pDev->max_seg - LCD_SPARE_PIXELS)) {
		++pDev->page ;
		if (pDev->page == pDev->max_page) {
			pDev->page = 0 ;
		}
		ssd1306SetPageAddr(pDev, pDev->page) ;
		ssd1306SetSegmentAddr(pDev, 0) ;
	}
	IF_EXEC_1(ssd1306DEBUG_TIMING, xClockTimerStop, clockTIMER_SSD1306_2) ;
	return cChr ;
}

int		ssd1306PutC(int cChr) { return ssd1306PutChar(&sSSD1306, cChr) ; }

void	ssd1306PutString(ssd1306_t * pDev, uint8_t * pString) { while(*pString) ssd1306PutChar(pDev, *pString++) ; }

int32_t ssd1306Diagnostics(ssd1306_t * pDev) {
#if 0
	ssd1306SetTextCursor(pDev, 0, 0) ; ssd1306PutString(pDev, (uint8_t *) "|00000000|") ;
	ssd1306SetTextCursor(pDev, 0, 1) ; ssd1306PutString(pDev, (uint8_t *) "+11111111+") ;
	ssd1306SetTextCursor(pDev, 0, 2) ; ssd1306PutString(pDev, (uint8_t *) "=22222222=") ;
	ssd1306SetTextCursor(pDev, 0, 3) ; ssd1306PutString(pDev, (uint8_t *) "[33333333]") ;
	ssd1306SetTextCursor(pDev, 0, 4) ; ssd1306PutString(pDev, (uint8_t *) "{44444444}") ;
	ssd1306SetTextCursor(pDev, 0, 5) ; ssd1306PutString(pDev, (uint8_t *) "(55555555)") ;
#endif
#if 0
	uint8_t cBuf[1+LCD_WIDTH] ;
	ssd1306SetTextCursor(pDev, 0, 0) ;
	cBuf[0]	= 0x40 ;								// sending data
	memset(&cBuf[1], 0x88, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xCC, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xEE, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x77, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x33, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x11, LCD_WIDTH) ;
	halI2C_Write(&pDev->sI2Cdev, cBuf, sizeof(cBuf)) ;
#endif
#if 0
	ssd1306Clear(pDev) ;
	ssd1306SetPageAddr(pDev, 0) ;
	ssd1306SetSegmentAddr(pDev, 0) ;
	for(int32_t i = 0; i < (LCD_COLUMNS * LCD_LINES); i++) {
		ssd1306PutChar(pDev, i + CHR_SPACE) ;
	}
#endif
	return erSUCCESS ;
}

int32_t	ssd1306Identify(uint8_t eChan, uint8_t Addr) {
	ssd1306_t * pDev = &sSSD1306 ;
	pDev->sI2Cdev.chanI2C	= eChan ;
	pDev->sI2Cdev.addrI2C	= Addr ;
	pDev->sI2Cdev.dlayI2C	= pdMS_TO_TICKS(100) ;

	// detect & verify existence.
	uint8_t ssd1306Status = ssd1306GetStatus(pDev);
	IF_PRINT(ssd1306DEBUG_CMDS, "ssd1306 Chan=%d Addr=%d Status = 0x%02x\n", eChan, Addr, ssd1306Status) ;

	// valid device signature not found
	if ((ssd1306Status & 0x03) != 0x03) {
		pDev->sI2Cdev.chanI2C	= 0 ;
		pDev->sI2Cdev.addrI2C	= 0 ;
		pDev->sI2Cdev.dlayI2C	= 0 ;
		return erFAILURE ;
	}

	// valid device signature found
	pDev->sI2Cdev.epidI2C.devclass	= devSSD1306 ;
	pDev->sI2Cdev.epidI2C.subclass	= subDSP64X48 ;
	pDev->sI2Cdev.epidI2C.epuri		= URI_UNKNOWN ;
	pDev->sI2Cdev.epidI2C.epunit	= UNIT_UNKNOWN ;
	ssd1306Init(pDev) ;
	return erSUCCESS ;
}

void	ssd1306Report(ssd1306_t * pDev) {
	PRINT("SSD1306: segment:%d max_seg:%d page:%d max_page:%d\n", pDev->segment, pDev->max_seg, pDev->page, pDev->max_page) ;
}

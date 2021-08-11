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

#include	"hal_variables.h"
#include	"ssd1306.h"
#include	"fonts.h"

#include	"printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include	"syslog.h"
#include	"systiming.h"

#include	"x_errors_events.h"

#include	<string.h>

#define	debugFLAG					0xF000

#define	debugCMDS					(debugFLAG & 0x0001)
#define	debugCONTRAST				(debugFLAG & 0x0002)
#define	debugMODE					(debugFLAG & 0x0004)

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

// ###################################### BUILD : CONFIG definitions ###############################

#define	ssd1306VERSION				"v0.0.1.4"

#define	ssd1306FONT					FONT5X7
#define	ssd1306FONT_HEIGHT			CONCAT2(ssd1306FONT, _HEIGHT)
#define	ssd1306FONT_WIDTH			CONCAT2(ssd1306FONT, _WIDTH)

// ####################################### LCD definitions #########################################

#define	LCD_TYPE_64_48				1					// Wemos D1 Mini OLED shield
#define	LCD_TYPE_128_64				2
#define	LCD_TYPE					LCD_TYPE_64_48

#if		(LCD_TYPE == LCD_TYPE_64_48)
	#define	LCD_WIDTH				64					// pixels horizontal
	#define	LCD_HEIGHT				48					// pixels vertical
#elif	(LCD_TYPE == LCD_TYPE_128_64)
	#define	LCD_WIDTH				128					// pixels horizontal
	#define	LCD_HEIGHT				64					// pixels vertical
#else
	#error "No/invalid LCD Type selected!!!"
#endif

#define	LCD_COLUMNS					(LCD_WIDTH / ssd1306FONT_WIDTH)
#define	LCD_LINES					(LCD_HEIGHT / ssd1306FONT_HEIGHT)
#define	LCD_SPARE_PIXELS			(LCD_WIDTH - (LCD_COLUMNS * ssd1306FONT_WIDTH))

#define	ANOMALY_OFFSET				32					// WEMOS Mini D1 OLED shield quirk

// ##################################### MACRO definitions #########################################

// Absolute maximum SSD1306 specs
#define	SSD1306_MAX_SEGMENT			128
#define	SSD1306_MAX_COLUMN			64
#define	SSD1306_MAX_PAGE			8

// Scrolling commands
#define ssd1306SCROLL_HOR_RIGHT		0x26
#define ssd1306SCROLL_HOR_LEFT		0x27
#define ssd1306SCROLL_VERT_RIGHT_HOR 0x29
#define ssd1306SCROLL_VERT_LEFT_HOR	0x2A
#define ssd1306SCROLL_DEACTIVATE	0x2E
#define ssd1306SCROLL_ACTIVATE		0x2F
#define ssd1306SCROLL_SET_VERT_AREA 0xA3

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

// ################################ private static variables ######################################

static ssd1306_t sSSD1306 = { .max_seg=LCD_WIDTH, .max_page=LCD_HEIGHT / ssd1306FONT_HEIGHT } ;

// ################################# private public variables #####################################

static void	ssd1306I2C_IO(uint8_t * pBuf, uint8_t Size) {
	int iRV ;
	if (Size == 1) {
		iRV = halI2C_Queue(sSSD1306.psI2C, i2cR_B, NULL, 0, pBuf, Size, (i2cq_p1_t) NULL, (i2cq_p2_t) NULL) ;
	} else {
		iRV = halI2C_Queue(sSSD1306.psI2C, i2cW_B, pBuf, Size, NULL, 0, (i2cq_p1_t) NULL, (i2cq_p2_t) NULL) ;
	}
	IF_myASSERT(debugRESULT, iRV >= erSUCCESS) ;
}

/**
 * ssd1306SendCommand_?() - send a single/dual/triple byte command(s) to the controller
 * @param	Cmd? - command(s) to send
 */
void ssd1306SendCommand_1(uint8_t Cmd1) {
	uint8_t cBuf[2] ;
	cBuf[0]	= 0x00 ;									// command following
	cBuf[1]	= Cmd1 ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;
}

void ssd1306SendCommand_2(uint8_t Cmd1, uint8_t Cmd2) {
	uint8_t cBuf[3] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;
}

void ssd1306SendCommand_3(uint8_t Cmd1, uint8_t Cmd2, uint8_t Cmd3) {
	uint8_t cBuf[4] ;
	cBuf[0]	= 0x00 ;									// commands following
	cBuf[1]	= Cmd1 ;
	cBuf[2]	= Cmd2 ;
	cBuf[3]	= Cmd3 ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;
}

uint8_t	ssd1306GetStatus(void) {
	ssd1306I2C_IO(&sSSD1306.status, SO_MEM(ssd1306_t, status)) ;
	return sSSD1306.status ;
}

/* Functions to configure the basic operation of the controller */

void ssd1306SetDisplayState(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306DISPLAYON : ssd1306DISPLAYOFF) ;
}

void ssd1306SetScrollState(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306SCROLL_ACTIVATE : ssd1306SCROLL_DEACTIVATE) ;
}

void ssd1306SetScanDirection(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306COMSCANINC : ssd1306COMSCANDEC) ;
}

/**
 *  @param	Mode = 0/horizontal  1/vertical  2/page
 */
void ssd1306SetMemoryMode(uint8_t Mode) {
	IF_myASSERT(debugPARAM, Mode < 0x03) ;
	sSSD1306.mem_mode	= Mode ;
	ssd1306SendCommand_2(ssd1306MEMORYMODE, Mode) ;
}

/**
 * @param	Offset - 0->63 Pan up or down
 */
void ssd1306SetOffset(uint8_t Offset) {
	IF_myASSERT(debugPARAM, Offset < SSD1306_MAX_COLUMN) ;
	ssd1306SendCommand_2(ssd1306SETDISPLAYOFFSET, Offset) ;
}

/**
 * @param	Contrast - 0->255 dim to bright
 */
int	ssd1306SetContrast(uint8_t Contrast) {
	if (Contrast > sSSD1306.MaxContrast) {
		Contrast = sSSD1306.MinContrast ;
	} else if (Contrast < sSSD1306.MinContrast) {
		Contrast = sSSD1306.MaxContrast ;
	}
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
	ssd1306SetDisplayState(Contrast == 0 ? 0 : 1) ;		// switch display off/on
	IF_PRINT(debugCONTRAST, "Contrast=0x%02X  PreCharge=0x%02X  Vcom=0x%02X\n", Contrast, PreCharge, NewVcom) ;
	return Contrast ;
}

void ssd1306SetInverse(uint8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02) ;
	ssd1306SendCommand_1(State ? ssd1306INVERTDISPLAY : ssd1306NORMALDISPLAY) ;
}

/* Functions to set the display graphics/pixel cursor and page position */

void ssd1306SetSegmentAddr(uint8_t Segment) {
	sSSD1306.segment = (Segment < sSSD1306.max_seg) ? Segment : 0 ;
	ssd1306SendCommand_3(ssd1306COLUMNADDR, Segment + ANOMALY_OFFSET, sSSD1306.max_seg + ANOMALY_OFFSET) ;
}

void ssd1306SetPageAddr(uint8_t Page) {
	sSSD1306.page = Page < sSSD1306.max_page ? Page : 0 ;
	ssd1306SendCommand_3(ssd1306PAGEADDR, Page, sSSD1306.max_page) ;
}

/* High level functions to control window/cursor/scrolling etc */

void ssd1306SetTextCursor(uint8_t X, uint8_t Y) {
	ssd1306SetPageAddr(Y) ;
	ssd1306SetSegmentAddr(X * ssd1306FONT_WIDTH) ;
}

void  ssd1306Clear(void) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, stSSD1306A) ;
	#define LINES	3									// 1=14,129  2=12,033  3=11,338  6=10,645 uSec
	uint8_t	cBuf[1+LCD_WIDTH*LINES] ;
	memset(cBuf, 0, sizeof(cBuf)) ;
	cBuf[0]	= 0x40 ;									// writing data
	for(int i = 0; i < LCD_LINES; i += LINES) {
		ssd1306SetTextCursor(0, i) ;
		ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;
	}
	ssd1306SetTextCursor(0, 0) ;
	IF_EXEC_1(debugTIMING, xSysTimerStop, stSSD1306A) ;
}

/**
 * @brief Write a single character at the current cursor position
 *
 * Using vertical addressing mode, we can have the hardware
 * automatically move to the next column after writing 7 pixels
 * in each page (rows).
 */
int	ssd1306PutChar(int cChr) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, stSSD1306B) ;
	const char * pFont = &font5X7[cChr * (ssd1306FONT_WIDTH - 1)] ;
	uint8_t	cBuf[ssd1306FONT_WIDTH + 1 ] ;
	IF_PRINT(debugCMDS,"%c : %02x-%02x-%02x-%02x-%02x\n", cChr, *pFont, *(pFont+1), *(pFont+2), *(pFont+3), *(pFont+4)) ;

	int	i = 0 ;
	cBuf[i++] = 0x40 ;									// data following
	for(; i < ssd1306FONT_WIDTH; cBuf[i++] = *pFont++) ;	// copy font bitmap across
	cBuf[i]	= 0x00 ;									// add blank separating segment
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;	// send the character

	// update the cursor location
	sSSD1306.segment	+= ssd1306FONT_WIDTH ;
	if (sSSD1306.segment >= (sSSD1306.max_seg - LCD_SPARE_PIXELS)) {
		++sSSD1306.page ;
		if (sSSD1306.page == sSSD1306.max_page) sSSD1306.page = 0 ;
		ssd1306SetPageAddr(sSSD1306.page) ;
		ssd1306SetSegmentAddr(0) ;
	}
	IF_EXEC_1(debugTIMING, xSysTimerStop, stSSD1306B) ;
	return cChr ;
}

void ssd1306PutString(const char * pString) { while(*pString) ssd1306PutChar(*pString++) ; }

/**
 * ssd1306ConfigMode() --  configure device functionality
 *
 * mode	/ssd1306 MinBright MaxBright BlankTime
 **/
int	ssd1306ConfigMode(rule_t * psRule) {
	uint8_t	AI = psRule->ActIdx ;
	uint32_t P0 = psRule->para.u32[AI][0] ;
	uint32_t P1 = psRule->para.u32[AI][1] ;
	uint32_t P2 = psRule->para.u32[AI][2] ;
	IF_PRINT(debugMODE, "ssd1306  P0=%d  P1=%d  P2=%d\n", P0, P1, P2) ;
	int iRV ;
	if ((P0 < P1) && (P1 <= 255) && (P2 <= 255)) {
		sSSD1306.MinContrast	= P0 ;
		sSSD1306.MaxContrast	= P1 ;
		sSSD1306.tBlank 		= P2 ;
		iRV = erSUCCESS ;
	} else {
		SET_ERRINFO("Invalid Min/Max Contrast or Blank") ;
		iRV = erSCRIPT_INV_PARA ;
	}
	// XXX Add support for enabling and disabling M90E26 info display
	return iRV ;
}

int	ssd1306Identify(i2c_di_t * psI2C_DI) {
	psI2C_DI->Delay	= pdMS_TO_TICKS(10) ;
	psI2C_DI->Test	= 1 ;
	sSSD1306.psI2C	= psI2C_DI ;
	ssd1306GetStatus();									// detect & verify existence.
	psI2C_DI->Test = 0 ;
	if ((sSSD1306.status & 0x03) != 0x03) {
		SL_ERR("I2C device at 0x%02X NOT SSD1306 (%02X)", psI2C_DI->Addr, sSSD1306.status) ;
		sSSD1306.psI2C	= NULL ;
		return erFAILURE ;
	}
	psI2C_DI->Type	= i2cDEV_SSD1306 ;
	// 10 bytes = 1mS @ 1000Khz, 250uS @ 400Khz
	psI2C_DI->Speed	= i2cSPEED_400 ;
	return erSUCCESS ;
}

int	ssd1306Config(i2c_di_t * psI2C_DI) {
	IF_SYSTIMER_INIT(debugTIMING, stSSD1306A, stMICROS, "SSD1306a", 2, 15) ;
	IF_SYSTIMER_INIT(debugTIMING, stSSD1306B, stMICROS, "SSD1306b", 2, 15) ;
	ssd1306ReConfig(psI2C_DI) ;
	return erSUCCESS ;
}

void ssd1306ReConfig(i2c_di_t * psI2C_DI) {
	ssd1306SendCommand_2(ssd1306SETMULTIPLEX, LCD_HEIGHT-1) ;
	ssd1306SetOffset(0) ;
	ssd1306SendCommand_1(ssd1306SETSTARTLINE | 0x0) ;
	ssd1306SendCommand_1(ssd1306SEGREMAP | 0x1) ;
	ssd1306SendCommand_1(ssd1306COMSCANDEC) ;
	ssd1306SendCommand_2(ssd1306SETCOMPINS, 0x12) ;
	ssd1306SendCommand_2(ssd1306SETDISPLAYCLOCKDIV, 0x80) ;
	ssd1306SendCommand_2(ssd1306CHARGEPUMP, ssd1306CHARGEPUMP_ON) ;
	ssd1306SetMemoryMode(0x00) ;
	ssd1306SendCommand_1(ssd1306DISPLAYALLON_RESUME) ;
	ssd1306SetInverse(0) ;
	ssd1306SetScrollState(0) ;
	ssd1306SetDisplayState(1) ;
	ssd1306Clear() ;
	sSSD1306.MinContrast	= 0 ;
	sSSD1306.MaxContrast	= 255 ;
	ssd1306SetContrast(128) ;
}

int ssd1306Diagnostics(i2c_di_t * psI2C_DI) {
	SL_INFO("ssd1306: Filling screen\n") ;
	ssd1306SetTextCursor(0, 0) ; ssd1306PutString("|00000000|") ;
	ssd1306SetTextCursor(0, 1) ; ssd1306PutString("+11111111+") ;
	ssd1306SetTextCursor(0, 2) ; ssd1306PutString("=22222222=") ;
	ssd1306SetTextCursor(0, 3) ; ssd1306PutString("[33333333]") ;
	ssd1306SetTextCursor(0, 4) ; ssd1306PutString("{44444444}") ;
	ssd1306SetTextCursor(0, 5) ; ssd1306PutString("(55555555)") ;

	SL_INFO("ssd1306: Writing bars\n") ;
	uint8_t cBuf[1+LCD_WIDTH] ;
	ssd1306SetTextCursor(0, 0) ;
	cBuf[0]	= 0x40 ;								// sending data
	memset(&cBuf[1], 0x88, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xCC, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0xEE, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x77, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x33, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	memset(&cBuf[1], 0x11, LCD_WIDTH) ;
	ssd1306I2C_IO(cBuf, sizeof(cBuf)) ;

	SL_INFO("ssd1306: Clearing the screen\n") ;
	ssd1306Clear() ;
	ssd1306SetPageAddr(0) ;
	ssd1306SetSegmentAddr(0) ;
	for(int i = 0; i < (LCD_COLUMNS * LCD_LINES); i++) {
		ssd1306PutChar(i + CHR_SPACE) ;
	}
	return erSUCCESS ;
}

void ssd1306Report(void) {
	printfx("SSD1306:  Seg:%d/%d  Page:%d/%d\n",
			sSSD1306.segment, sSSD1306.max_seg, sSSD1306.page, sSSD1306.max_page) ;
}

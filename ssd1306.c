/**
 * ssd1306.c
 * Copyright (c) 2014-22 Andre M. Maree / KSS Technologies (Pty) Ltd.
 * https://github.com/wtfuzz/ssd1306_text
 */

#include	"hal_variables.h"
#include	"ssd1306.h"
#include	"printfx.h"									// +x_definitions +stdarg +stdint +stdio
#include	"syslog.h"
#include	"systiming.h"
#include	"x_errors_events.h"

// ######################################## Build macros ###########################################

#define	debugFLAG					0xF000

#define	debugTIMING					(debugFLAG_GLOBAL & debugFLAG & 0x1000)
#define	debugTRACK					(debugFLAG_GLOBAL & debugFLAG & 0x2000)
#define	debugPARAM					(debugFLAG_GLOBAL & debugFLAG & 0x4000)
#define	debugRESULT					(debugFLAG_GLOBAL & debugFLAG & 0x8000)

// ########################################### Macros ##############################################

// Absolute maximum SSD1306 specs
#define	SSD1306_MAX_SEGMENT								128
#define	SSD1306_MAX_COLUMN								64
#define	SSD1306_MAX_PAGE								8

// Scrolling commands
#define ssd1306SCROLL_HOR_RIGHT							0x26
#define ssd1306SCROLL_HOR_LEFT							0x27
#define ssd1306SCROLL_VERT_RIGHT_HOR 					0x29
#define ssd1306SCROLL_VERT_LEFT_HOR						0x2A
#define ssd1306SCROLL_DEACTIVATE						0x2E
#define ssd1306SCROLL_ACTIVATE							0x2F
#define ssd1306SCROLL_SET_VERT_AREA 					0xA3

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

// ###################################### Private variables ########################################

ssd1306_t sSSD1306 = { 0 };
u8_t BufBits[1 + (LCD_WIDTH_PX * LCD_MAX_ROW)];

// ###################################### Public variables #########################################


// ######################################## Private APIs ###########################################

static void	ssd1306I2C_IO(u8_t * pBuf, size_t Size) {
	int iRV;
	if (Size == 1) {
		iRV = halI2C_Queue(sSSD1306.psI2C, i2cR_B, NULL, 0, pBuf, Size, (i2cq_p1_t) NULL, (i2cq_p2_t) NULL);
	} else {
		iRV = halI2C_Queue(sSSD1306.psI2C, i2cW_B, pBuf, Size, NULL, 0, (i2cq_p1_t) NULL, (i2cq_p2_t) NULL);
	}
	if (iRV < erSUCCESS)
		xSyslogError(__FUNCTION__, iRV);
}

// ######################################## Public APIs ############################################

/**
 * ssd1306SendCommand_?() - send a single/dual/triple byte command(s) to the controller
 * @param	Cmd? - command(s) to send
 */
void ssd1306SendCommand_1(u8_t Cmd1) {
	u8_t cBuf[2];
	cBuf[0]	= 0x00;									// command following
	cBuf[1]	= Cmd1;
	ssd1306I2C_IO(cBuf, sizeof(cBuf));
}

void ssd1306SendCommand_2(u8_t Cmd1, u8_t Cmd2) {
	u8_t cBuf[3];
	cBuf[0]	= 0x00;									// commands following
	cBuf[1]	= Cmd1;
	cBuf[2]	= Cmd2;
	ssd1306I2C_IO(cBuf, sizeof(cBuf));
}

void ssd1306SendCommand_3(u8_t Cmd1, u8_t Cmd2, u8_t Cmd3) {
	u8_t cBuf[4];
	cBuf[0]	= 0x00;									// commands following
	cBuf[1]	= Cmd1;
	cBuf[2]	= Cmd2;
	cBuf[3]	= Cmd3;
	ssd1306I2C_IO(cBuf, sizeof(cBuf));
}

u8_t ssd1306GetStatus(void) {
	ssd1306I2C_IO(&sSSD1306.status, SO_MEM(ssd1306_t, status));
	return sSSD1306.status;
}

/* Functions to configure the basic operation of the controller */

void ssd1306SetDisplayState(u8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02);
	ssd1306SendCommand_1(State ? ssd1306DISPLAYON : ssd1306DISPLAYOFF);
}

void ssd1306SetScrollState(u8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02);
	ssd1306SendCommand_1(State ? ssd1306SCROLL_ACTIVATE : ssd1306SCROLL_DEACTIVATE);
}

void ssd1306SetScanDirection(u8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02);
	ssd1306SendCommand_1(State ? ssd1306COMSCANINC : ssd1306COMSCANDEC);
}

/**
 *  @param	Mode = 0/horizontal  1/vertical  2/page
 */
void ssd1306SetMemoryMode(u8_t Mode) {
	IF_myASSERT(debugPARAM, Mode < 0x03);
	sSSD1306.mem_mode = Mode;
	ssd1306SendCommand_2(ssd1306MEMORYMODE, Mode);
}

/**
 * @param	Offset - 0->63 Pan up or down
 */
void ssd1306SetOffset(u8_t Offset) {
	IF_myASSERT(debugPARAM, Offset < SSD1306_MAX_COLUMN);
	ssd1306SendCommand_2(ssd1306SETDISPLAYOFFSET, Offset);
}

/**
 * @param	Contrast - 0->255 dim to bright
 */
int	ssd1306SetContrast(u8_t Contrast) {
	if (Contrast > sSSD1306.MaxContrast) {
		Contrast = sSSD1306.MinContrast;
	} else if (Contrast < sSSD1306.MinContrast) {
		Contrast = sSSD1306.MaxContrast;
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
	u8_t RelContrast = Contrast >> 4;
	#if 0
	u8_t PreCharge = RelContrast == 0x00 ? 0x10 : RelContrast << 4;
	PreCharge |= RelContrast  == 0x0F ? 0x01 : 0x0F - RelContrast;
	#elif 1
	u8_t PreCharge = RelContrast << 4 | RelContrast;
	PreCharge = PreCharge == 0x00 ? 0x11 : PreCharge;
	#endif
	ssd1306SendCommand_2(ssd1306SETPRECHARGE, PreCharge);
	ssd1306SendCommand_2(ssd1306SETCONTRAST, Contrast);
	u8_t NewVcom = RelContrast < 0x06 ? 0x00 : RelContrast < 0x0B ? 0x20 : 0x30;
	ssd1306SendCommand_2(ssd1306SETVCOMDESELECT, NewVcom);
	ssd1306SetDisplayState(Contrast == 0 ? 0 : 1);		// switch display off/on
//	P("Contrast=0x%02X  PreCharge=0x%02X  Vcom=0x%02X\r\n", Contrast, PreCharge, NewVcom);
	return Contrast;
}

void ssd1306SetInverse(u8_t State) {
	IF_myASSERT(debugPARAM, State < 0x02);
	ssd1306SendCommand_1(State ? ssd1306INVERTDISPLAY : ssd1306NORMALDISPLAY);
}

/* Functions to set the display graphics/pixel cursor and page position */
void ssd1306SetSegmentAddr(u8_t Segment) {
	ssd1306SendCommand_3(ssd1306COLUMNADDR, (sSSD1306.cur_seg = Segment % LCD_WIDTH_PX) + LCD_ANOMALY_PAD, LCD_WIDTH_PX + LCD_ANOMALY_PAD);
}

void ssd1306SetPageAddr(u8_t Page) {
	ssd1306SendCommand_3(ssd1306PAGEADDR, sSSD1306.cur_row = Page % LCD_MAX_ROW, LCD_MAX_ROW);
}

/* High level functions to control window/cursor/scrolling etc */

void ssd1306SetTextCursor(u8_t X, u8_t Y) {
	ssd1306SetPageAddr(Y);
	ssd1306SetSegmentAddr((X * ssd1306FONT_WIDTH) + LCD_LEFT_PAD);
}

void ssd1306Clear(void) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, stSSD1306A);
	memset(BufBits, 0, sizeof(BufBits));
	BufBits[0] = 0x40;									// writing data
	ssd1306SetTextCursor(0, 0);
	ssd1306I2C_IO(BufBits, sizeof(BufBits));
	ssd1306SetTextCursor(0, 0);
	IF_EXEC_1(debugTIMING, xSysTimerStop, stSSD1306A);
}

static bool ssd1306StepCursor(bool DoUpdate) {
	sSSD1306.cur_seg += ssd1306FONT_WIDTH;				// update the cursor location
	if (sSSD1306.cur_seg >= (LCD_WIDTH_PX - LCD_RIGHT_PAD)) {
		++sSSD1306.cur_row;
		if (sSSD1306.cur_row == LCD_MAX_ROW)
			sSSD1306.cur_row = 0;
		if (DoUpdate) {
			ssd1306SetPageAddr(sSSD1306.cur_row);
			ssd1306SetSegmentAddr(LCD_LEFT_PAD);
		} else {
			sSSD1306.cur_seg = LCD_LEFT_PAD;
		}
		return true;
	}
	return false;
}

/**
 * @brief Write a single character at the current cursor position
 *
 * Using vertical addressing mode, we can have the hardware
 * automatically move to the next column after writing 7 pixels
 * in each page (rows).
 */
int	ssd1306PutChar(int cChr) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, stSSD1306B);
	const char * pFont = &font5X7[cChr * (ssd1306FONT_WIDTH - 1)];
	u8_t cBuf[ssd1306FONT_WIDTH + 1];
	int	i = 0;
	cBuf[i++] = 0x40;									// data following
	for(; i < ssd1306FONT_WIDTH; cBuf[i++] = *pFont++);	// copy font bitmap across
	cBuf[i]	= 0x00;										// add blank separating segment
	ssd1306I2C_IO(cBuf, sizeof(cBuf));					// send the character
	ssd1306StepCursor(true);
	IF_EXEC_1(debugTIMING, xSysTimerStop, stSSD1306B);
	return cChr;
}

void ssd1306PutString(const char * pString) {
	IF_EXEC_1(debugTIMING, xSysTimerStart, stSSD1306B);
	int bbi = 0, ci = 0;
	BufBits[bbi++] = 0x40;								// data following
	while (*pString && ci < LCD_MAX_CHR) {
		int cChr = *pString++;
		const char * pFont = &font5X7[cChr * (ssd1306FONT_WIDTH - 1)];
		for(int fi = 0; fi < (ssd1306FONT_WIDTH-1); BufBits[bbi++] = pFont[fi++]);
		BufBits[bbi++] = 0x00;							// add blank separating segment
		++ci;
		if (ssd1306StepCursor(false))
			for(int fi = 0; fi < 5; BufBits[bbi++] = 0, fi++);
	}
	if (ci) {
		ssd1306I2C_IO(BufBits, bbi);					// send the character(s)
	}
	// TODO: Add support to update the cursor locations
	ssd1306SendCommand_3(ssd1306PAGEADDR, sSSD1306.cur_row, LCD_MAX_ROW);
	ssd1306SendCommand_3(ssd1306COLUMNADDR, sSSD1306.cur_seg + LCD_ANOMALY_PAD, LCD_WIDTH_PX + LCD_ANOMALY_PAD);
	IF_EXEC_1(debugTIMING, xSysTimerStop, stSSD1306B);
}

int	ssd1306Identify(i2c_di_t * psI2C_DI) {
	psI2C_DI->TRXmS	= 10;
	psI2C_DI->CLKuS = 400;			// Max 13000 (13mS)
	psI2C_DI->Test	= 1;
	sSSD1306.psI2C	= psI2C_DI;
	ssd1306GetStatus();									// detect & verify existence.
	psI2C_DI->Test = 0;
	if ((sSSD1306.status & 0x03) != 0x03) {
		SL_ERR("I2C device at 0x%02X NOT SSD1306 (%02X)", psI2C_DI->Addr, sSSD1306.status);
		sSSD1306.psI2C	= NULL;
		return erFAILURE;
	}
	psI2C_DI->Type	= i2cDEV_SSD1306;
	// 10 bytes = 1mS @ 100Khz, 250uS @ 400Khz
	psI2C_DI->Speed	= i2cSPEED_400;
	return erSUCCESS;
}

int	ssd1306Config(i2c_di_t * psI2C_DI) {
	IF_SYSTIMER_INIT(debugTIMING, stSSD1306A, stMICROS, "SSD1306a", 1500, 15000);
	IF_SYSTIMER_INIT(debugTIMING, stSSD1306B, stMICROS, "SSD1306b", 300, 3000);
	ssd1306ReConfig(psI2C_DI);
	return erSUCCESS;
}

void ssd1306ReConfig(i2c_di_t * psI2C_DI) {
	ssd1306SendCommand_2(ssd1306SETMULTIPLEX, LCD_HEIGHT_PX-1);
	ssd1306SetOffset(0);
	ssd1306SendCommand_1(ssd1306SETSTARTLINE | 0x0);
	ssd1306SendCommand_1(ssd1306SEGREMAP | 0x1);
	ssd1306SendCommand_1(ssd1306COMSCANDEC);
	ssd1306SendCommand_2(ssd1306SETCOMPINS, 0x12);
	ssd1306SendCommand_2(ssd1306SETDISPLAYCLOCKDIV, 0x80);
	ssd1306SendCommand_2(ssd1306CHARGEPUMP, ssd1306CHARGEPUMP_ON);
	ssd1306SetMemoryMode(0x00);
	ssd1306SendCommand_1(ssd1306DISPLAYALLON_RESUME);
	ssd1306SetInverse(0);
	ssd1306SetScrollState(0);
	ssd1306SetDisplayState(1);
	ssd1306Clear();
	sSSD1306.MinContrast = 0;
	sSSD1306.MaxContrast = 255;
	ssd1306SetContrast(128);
}

int ssd1306Diagnostics(i2c_di_t * psI2C_DI) {
	SL_DBG("ssd1306: Filling screen\r\n");
	ssd1306SetTextCursor(0, 0); ssd1306PutString("|00000000|");
	ssd1306SetTextCursor(0, 1); ssd1306PutString("+11111111+");
	ssd1306SetTextCursor(0, 2); ssd1306PutString("=22222222=");
	ssd1306SetTextCursor(0, 3); ssd1306PutString("[33333333]");
	ssd1306SetTextCursor(0, 4); ssd1306PutString("{44444444}");
	ssd1306SetTextCursor(0, 5); ssd1306PutString("(55555555)");

	SL_DBG("ssd1306: Writing bars\r\n");
	u8_t cBuf[1+LCD_WIDTH_PX];
	ssd1306SetTextCursor(0, 0);
	cBuf[0]	= 0x40;								// sending data
	memset(&cBuf[1], 0x88, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	memset(&cBuf[1], 0xCC, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	memset(&cBuf[1], 0xEE, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	memset(&cBuf[1], 0x77, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	memset(&cBuf[1], 0x33, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	memset(&cBuf[1], 0x11, LCD_WIDTH_PX);
	ssd1306I2C_IO(cBuf, sizeof(cBuf));

	SL_DBG("ssd1306: Clearing the screen\r\n");
	ssd1306Clear();
	for(int i = 0; i < (LCD_MAX_COL * LCD_MAX_ROW); i++)
		ssd1306PutChar(i + CHR_SPACE);
	return erSUCCESS;
}

void ssd1306Report(void) {
	P("SSD1306:  Seg:%d/%d  Page:%d/%d\r\n", sSSD1306.cur_seg, LCD_WIDTH_PX, sSSD1306.cur_row, LCD_MAX_ROW);
}

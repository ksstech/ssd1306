/**
 * ssd1306.h
 */

#pragma	once

#include "hal_i2c.h"
#include "fonts.h"

#include <stdint.h>

/*	Wemos 0.96" OLED
 	D3	IO17	BtnA default	M90E26 CS1 conflict
 	D4	IO16	BtnB default
 	D5	IO18					M90E26 SCLK
 	D6	IO19					M90E26 MISO
 	D7	IO23					M90E26 MOSI
 */

#ifdef __cplusplus
extern "C" {
#endif

// ##################################### BUILD definitions #########################################


// ##################################### MACRO definitions #########################################

#define	SSD1306_ADDR_PRI						0x3C
#define	SSD1306_ADDR_SEC						0x3D

#define	ssd1306FONT					FONT5X7
#define	ssd1306FONT_HEIGHT			8
#define	ssd1306FONT_WIDTH			6

#define	LCD_TYPE_64_48				1					// Wemos D1 Mini OLED shield
#define	LCD_TYPE_128_64				2
#define	LCD_TYPE					LCD_TYPE_64_48

#if	(LCD_TYPE == LCD_TYPE_64_48)
	#define	LCD_WIDTH_PX			64					// pixels horizontal
	#define	LCD_HEIGHT_PX			48					// pixels vertical
	#define LCD_MAX_ROW				(LCD_HEIGHT_PX / ssd1306FONT_HEIGHT)
	#define LCD_MAX_COL				(LCD_WIDTH_PX / ssd1306FONT_WIDTH)
	#define LCD_MAX_CHR				(LCD_MAX_ROW * LCD_MAX_COL)
	#define LCD_SPARE_PX			(LCD_WIDTH_PX - (LCD_MAX_COL * ssd1306FONT_WIDTH))
	#define LCD_LEFT_PAD			0
	#define LCD_RIGHT_PAD			(LCD_SPARE_PX - LCD_LEFT_PAD)
	#define	LCD_ANOMALY_PAD			32					// WEMOS Mini D1 OLED shield quirk
#elif(LCD_TYPE == LCD_TYPE_128_64)
	#define	LCD_WIDTH_PX				128					// pixels horizontal
	#define	LCD_HEIGHT_PX				64					// pixels vertical
#else
	#error "No/invalid LCD Type selected!!!"
#endif


// ######################################## Enumerations ###########################################

enum { eDISPLAY };										// sensor values display on/off

// ####################################### structures ##############################################

typedef struct __attribute__((packed)) ssd1306_t {
	i2c_di_t * psI2C;
	u8_t MinContrast;
	u8_t MaxContrast;
	u8_t NowContrast;
	u8_t tBlank;
	u8_t status;
	struct __attribute__((packed)) {
		u8_t cur_seg :8;								// current pixel/segment horizontal
		u8_t cur_col :5;								// current text column (0 -> 9/20)
		u8_t cur_row :3;								// current row ie group of 8 pixel rows
		u8_t mem_mode :2;								// 0=horizontal  1=vertical  2=page
		u8_t state:1;
	};
} ssd1306_t;
DUMB_STATIC_ASSERT(sizeof(ssd1306_t) == 12);

// ######################################## public variables #######################################

extern ssd1306_t sSSD1306;

// ################################### EXTERNAL FUNCTIONS ##########################################

int	ssd1306SetContrast(u8_t Contrast);
int ssd1306GetContrast(void);
int ssd1306StepContrast(s8_t Step);
void ssd1306SetDisplayState(bool State);
bool ssd1306GetDisplayState(void);
void ssd1306SetTextCursor(u8_t X, u8_t Y);
void ssd1306Clear(void);
int	ssd1306PutChar(int cChr);
void ssd1306PutString(const char *);

int ssd1306Identify(i2c_di_t * psI2C_DI);
int ssd1306Config(i2c_di_t * psI2C_DI);
void ssd1306ReConfig(i2c_di_t * psI2C_DI);
int ssd1306Diagnostics(i2c_di_t * psI2C_DI);

void ssd1306Report(void);

#ifdef __cplusplus
}
#endif


/**
 * ssd1306.h
 */

#pragma	once

#include	"endpoints.h"
#include	"hal_i2c.h"
#include	"rules_engine.h"

#include	<stdint.h>

// ##################################### BUILD definitions #########################################


// ##################################### MACRO definitions #########################################

#define	SSD1306_ADDR_PRI						0x3C
#define	SSD1306_ADDR_SEC						0x3D

// ######################################## Enumerations ###########################################

enum {													// supported mode options
	eDISPLAY,											// sensor values display on/off
} ;

// ####################################### structures ##############################################

typedef struct __attribute__((packed)) ssd1306_t {
	i2c_di_t *	psI2C ;									// size = 4
	uint8_t	segment	: 8 ;								// pixel horizontal
	uint8_t	max_seg	: 8 ;								// max pixel 0->64/128 ?
	struct __attribute__((packed)) {
		uint16_t	page	: 7 ;
		uint16_t	max_page: 7 ;						// max lines 0->48/64 ?
		uint16_t	mem_mode: 2 ;						// 0=horizontal  1/vertical  2/page
	} ;
	uint8_t		status ;
	uint8_t		MinContrast ;
	uint8_t		MaxContrast ;
	uint8_t		tBlank ;
} ssd1306_t ;
DUMB_STATIC_ASSERT(sizeof(ssd1306_t) == 12) ;

// ######################################## public variables #######################################


// ################################### EXTERNAL FUNCTIONS ##########################################

int	ssd1306SetContrast(uint8_t Contrast) ;
void ssd1306SetDisplayState(uint8_t State) ;
void ssd1306SetTextCursor(uint8_t X, uint8_t Y) ;
int	ssd1306PutChar(int cChr) ;
void ssd1306PutString(const char *) ;

int	ssd1306ConfigMode(rule_t * psRule) ;

int ssd1306Identify(i2c_di_t * psI2C_DI) ;
int ssd1306Config(i2c_di_t * psI2C_DI) ;
void ssd1306ReConfig(i2c_di_t * psI2C_DI) ;
int ssd1306Diagnostics(i2c_di_t * psI2C_DI) ;

void ssd1306Report(void) ;

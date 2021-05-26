/**
 * ssd1306.h
 */

#pragma	once

#include	"rules_engine.h"
#include	"endpoint_struct.h"
#include	"hal_i2c.h"

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
	i2c_dev_info_t *	psI2C ;							// size = 4
//	SemaphoreHandle_t	mux ;
	uint8_t		segment ;								// pixel horizontal
	uint8_t		max_seg ;								// max pixel 0->64/128 ?
	uint8_t		page ;
	uint8_t		max_page ;								// max lines 0->48/64 ?
	uint8_t		status ;
	uint8_t		MinContrast ;
	uint8_t		MaxContrast ;
	uint8_t		tBlank ;
	uint8_t		mem_mode	: 3 ;
} ssd1306_t ;
DUMB_STATIC_ASSERT(sizeof(ssd1306_t) == 13) ;

// ######################################## public variables #######################################


// ################################### EXTERNAL FUNCTIONS ##########################################

int32_t	ssd1306SetContrast(uint8_t Contrast) ;
void	ssd1306SetDisplayState(uint8_t State) ;
void	ssd1306SetTextCursor(uint8_t X, uint8_t Y) ;
int		ssd1306PutChar(int cChr) ;
void	ssd1306PutString(const char *) ;

int32_t	ssd1306ConfigMode(rule_t * psRule) ;

int32_t ssd1306Identify(i2c_dev_info_t * psI2C_DI) ;
int32_t ssd1306Config(i2c_dev_info_t * psI2C_DI) ;
int32_t ssd1306Diagnostics(i2c_dev_info_t * psI2C_DI) ;

void	ssd1306Report(void) ;

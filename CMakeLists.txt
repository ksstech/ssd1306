# SSD1306
set( srcs "ssd1306.c" )
set( include_dirs "." )
set( requires endpoints hal_esp32 rules )
set( priv_requires common fonts printfx syslog systiming )
idf_component_register(
	SRCS ${srcs} 
	INCLUDE_DIRS ${include_dirs} 
	REQUIRES ${requires} 
	PRIV_REQUIRES ${priv_requires}
)

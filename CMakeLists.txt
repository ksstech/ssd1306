idf_component_register(	SRCS ssd1306.c 
						INCLUDE_DIRS . 
						REQUIRES
						PRIV_REQUIRES fonts endpoints syslog printf common systiming hal_esp32
						)

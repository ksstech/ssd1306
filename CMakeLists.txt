idf_component_register(
	SRCS "ssd1306.c"
	INCLUDE_DIRS "."
	REQUIRES hal_esp32
	PRIV_REQUIRES fonts printfx systiming
)

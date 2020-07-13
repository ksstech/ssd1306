set(srcs "ssd1306.c" )
set(include_dirs "." )
#set(priv_include_dirs "." )
set(requires hal_esp32 )
set(priv_requires fonts endpoints syslog printf common systiming )
#set(priv_requires ${priv_requires} m90e26 )
idf_component_register( SRCS ${srcs}
						INCLUDE_DIRS ${include_dirs} 
#						PRIV_INCLUDE_DIRS ${priv_include_dirs} 
						REQUIRES ${requires} 
						PRIV_REQUIRES ${priv_requires}
						)

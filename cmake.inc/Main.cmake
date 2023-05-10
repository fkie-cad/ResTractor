set(APP resTractor)
add_executable(
	${APP}
	src/main.c
	)
target_sources(${APP} PRIVATE
	${APP_SRC}
	)
target_link_libraries(${APP} PRIVATE
	${HEADER_PARSER_ST}
	)

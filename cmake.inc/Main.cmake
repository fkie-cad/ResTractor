set(EXE resTractor)
add_executable(
	${EXE}
	src/resTractor.c
	src/utils/fifo/Fifo.c
	)
target_sources(${EXE} PRIVATE
	${EXE_SRC}
	)

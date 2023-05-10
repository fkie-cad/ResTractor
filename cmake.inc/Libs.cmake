function(checkLibExistence NAME TARGET IS_REQUIRED)
	if (EXISTS ${TARGET})
		message("-- ${NAME} lib found: ${TARGET}")
	else()
		if ( IS_REQUIRED)
			message(FATAL_ERROR "-- ${NAME} lib not found: ${TARGET}")
		else()
			message("-- ${NAME} lib not found: ${TARGET}")
		endif()
	endif()
endfunction(checkLibExistence)



enable_testing()
find_package(GTest QUIET)
#include_directories(${GTEST_INCLUDE_DIRS})
message("-- GTEST_FOUND: ${GTEST_FOUND} ${GTEST_BOTH_LIBRARIES}")



set(LIB_NAME headerparser)

set(HP_ST_FULL_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}${LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
set(HEADER_PARSER_ST ${CMAKE_SOURCE_DIR}/res/lib/${HP_ST_FULL_NAME})
checkLibExistence(${LIB_NAME} ${HEADER_PARSER_ST} true)

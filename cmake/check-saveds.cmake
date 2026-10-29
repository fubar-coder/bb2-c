# __saveds must load a4 (jsr ___restore_a4).  m68k-amigaos-gcc before
# 16.2-rc21 (AmigaPorts/gcc#141) accepts __saveds and generates nothing: a
# function entered with someone else's a4 then reads its data through the
# caller's a4, without a warning.
#   include(): amiga_check_saveds(CC SOURCE) - FATAL_ERROR if not
#   cmake -DCC=... -DSRC=... -P check-saveds.cmake (the test with
#   cmake/saveds/broken.c, a function without __saveds)
function(amiga_check_saveds cc src)
	execute_process(COMMAND ${cc} -m68000 -noixemul -Os -fbaserel -S -o - ${src}
		OUTPUT_VARIABLE asm ERROR_VARIABLE err RESULT_VARIABLE rc)
	if(NOT rc EQUAL 0)
		message(FATAL_ERROR "${cc}: cannot compile ${src}: ${err}")
	endif()
	if(NOT asm MATCHES "jsr[ \t]+___restore_a4")
		message(FATAL_ERROR "${cc}: __saveds does not load a4 (no jsr ___restore_a4 "
			"in ${src}); m68k-amigaos-gcc 16.2-rc21 or later is needed")
	endif()
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	amiga_check_saveds(${CC} ${SRC})
endif()

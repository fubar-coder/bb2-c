# ctest variant_*: the libraries of a variant (libs/variants, built to
# BLITZ_VARIANTS_DIR/NAME) against the default build.
#   cmake -DMODE=same -DVARIANT=DIR -DDEFAULT=DIR -P check.cmake
#       acidlibs and deflibs of VARIANT byte-identical to DEFAULT's
#   cmake -DMODE=renumbered -DVARIANT=DIR -DBBSRC=... -DBLITZ2=...
#         -DTOKENS=ted-tokens.txt -DMAP=OLD:NEW;... -DOBJ=FILE
#         -DCALL=HEX -DOLDCALL=HEX -DOUT=FILE -P check.cmake
#       the token list of BLITZ2 with VARIANT's acidlibs and deflibs is
#       TOKENS with the tokens of every library OLD moved to NEW (same
#       order, same names); the library file OBJ calls another library
#       through the placeholder CALL (ALibJsr), no longer through OLDCALL
#       (without -DOBJ: no call to check, no library calls a moved one)
if(MODE STREQUAL "same")
	foreach(f acidlibs deflibs)
		execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${VARIANT}/${f} ${DEFAULT}/${f}
			RESULT_VARIABLE diff)
		if(diff)
			message(FATAL_ERROR "${VARIANT}/${f} differs from ${DEFAULT}/${f}")
		endif()
	endforeach()
	message(STATUS "${VARIANT}: acidlibs and deflibs as in ${DEFAULT}")
	return()
endif()
if(NOT MODE STREQUAL "renumbered")
	message(FATAL_ERROR "MODE same or renumbered")
endif()

execute_process(COMMAND ${BBSRC} tokens -o ${OUT} ${BLITZ2} ${VARIANT}/acidlibs ${VARIANT}/deflibs
	RESULT_VARIABLE rc)
if(rc)
	message(FATAL_ERROR "bbsrc tokens: exit ${rc}")
endif()
file(STRINGS ${TOKENS} want)
# literallib's tokens ($9081-$90FF, #1018) are not in TOKENS (the
# original's list): the variant has them like the default build.  Their
# names are bytes from $81, which file(STRINGS) splits: the lines are
# taken out of the text first (OUT.nolit)
file(READ ${OUT} text)
string(REGEX REPLACE "(^|\n)90[89A-F][0-9A-F] [^\n]*" "" text "${text}")
file(WRITE ${OUT}.nolit "${text}")
file(STRINGS ${OUT}.nolit have)
list(LENGTH want n)
list(LENGTH have m)
if(NOT n EQUAL m)
	message(FATAL_ERROR "${OUT}: ${m} tokens, ${n} expected")
endif()
set(moved 0)
math(EXPR last "${n} - 1")
foreach(i RANGE ${last})
	list(GET want ${i} w)
	list(GET have ${i} h)
	string(SUBSTRING "${w}" 0 4 hex)
	math(EXPR num "0x${hex} & 0x7FFF")
	math(EXPR lib "${num} >> 7")
	foreach(m IN LISTS MAP)
		string(REPLACE ":" ";" m "${m}")
		list(GET m 0 old)
		list(GET m 1 new)
		if(lib EQUAL old AND num LESS 0x7F00)
			math(EXPR num "0x8000 | (${new} << 7) | (${num} & 0x7F)" OUTPUT_FORMAT HEXADECIMAL)
			string(SUBSTRING "${num}" 2 -1 num)
			string(TOUPPER "${num}" num)
			string(SUBSTRING "${w}" 4 -1 rest)
			set(w "${num}${rest}")
			math(EXPR moved "${moved} + 1")
		endif()
	endforeach()
	if(NOT "${w}" STREQUAL "${h}")
		message(FATAL_ERROR "token ${i}: \"${h}\", \"${w}\" expected")
	endif()
endforeach()
if(NOT moved)
	message(FATAL_ERROR "no token of ${MAP} in ${TOKENS}")
endif()
if(NOT OBJ)
	message(STATUS "${VARIANT}: ${moved} tokens moved (${MAP})")
	return()
endif()
file(READ ${OBJ} hex HEX)
string(FIND "${hex}" "${CALL}" a)
string(FIND "${hex}" "${OLDCALL}" b)
if(a LESS 0 OR NOT b LESS 0)
	message(FATAL_ERROR "${OBJ}: call ${CALL} missing or old call ${OLDCALL} still there")
endif()
message(STATUS "${VARIANT}: ${moved} tokens moved (${MAP}), ${OBJ} calls ${CALL}")

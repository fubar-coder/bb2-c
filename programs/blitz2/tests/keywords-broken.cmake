# keywords-broken.cmake - the broken-input test of the check of the
# compiler's keywords in blitz2's build step (CMakeLists.txt here): a copy
# of the token list in WORK with a changed keyword ("Wend" made "WEnd"),
# checked by toklist json --check against the committed file: the check
# must fail (exit status 1), name the line of the keyword in the committed
# file, count one line, and say how to update the file.
#   cmake -DTOKLIST=toklist -DJSON=blitz2.json -DLIST=toklist.h -DWORK=DIR -P keywords-broken.cmake
foreach(v TOKLIST JSON LIST WORK)
	if(NOT DEFINED ${v})
		message(FATAL_ERROR "keywords-broken: -D${v}=... missing")
	endif()
endforeach()
file(MAKE_DIRECTORY ${WORK})
file(READ ${LIST} list)
file(READ ${JSON} json)
set(bad "")

string(FIND "${list}" "\"Wend\"" at)
if(at LESS 0)
	message(FATAL_ERROR "keywords-broken: no keyword \"Wend\" in ${LIST}")
endif()
string(REPLACE "\"Wend\"" "\"WEnd\"" changed "${list}")
file(WRITE ${WORK}/toklist.h "${changed}")

# the line of the keyword in the committed file
string(FIND "${json}" "{\"name\": \"Wend\"," at)
if(at LESS 0)
	message(FATAL_ERROR "keywords-broken: no keyword \"Wend\" in ${JSON}")
endif()
string(SUBSTRING "${json}" 0 ${at} head)
string(REGEX MATCHALL "\n" nl "${head}")
list(LENGTH nl line)
math(EXPR line "${line} + 1")

execute_process(COMMAND ${TOKLIST} json --check ${JSON} ${WORK}/toklist.h
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT rc EQUAL 1)
	string(APPEND bad "exit status ${rc}, wanted 1\n")
endif()
foreach(want "${JSON}:${line}: differs from the generated file: "
		"generated:     {\"name\": \"WEnd\","
		"${JSON}: lines that differ from the generated file: 1\n"
		"build the target blitz2_keywords_update")
	string(FIND "${err}" "${want}" found)
	if(found LESS 0)
		string(APPEND bad "not in the message: ${want}\n")
	endif()
endforeach()

if(bad)
	message(FATAL_ERROR "keywords-broken:\n${bad}output:\n${out}${err}")
endif()
message("keywords-broken: as expected")

# check-cache-variable-test.cmake - the test of check-cache-variable.cmake
# for the cache variable NAME: the environment variable set and the cache
# variable empty fails with the message naming the variable; the cache
# variable set (also with the environment variable), the environment
# variable empty or unset pass.
#   cmake -DNAME=VAR -DSCRIPT=check-cache-variable.cmake -P check-cache-variable-test.cmake
foreach(v NAME SCRIPT)
	if(NOT ${v})
		message(FATAL_ERROR "check-cache-variable-test.cmake: -D${v}=... missing")
	endif()
endforeach()
# ENV: the value of the environment variable (unset: none), VALUE: the
# cache variable, WANT pass|fail
function(check env value want)
	set(prefix ${CMAKE_COMMAND} -E env)
	if(env STREQUAL "unset")
		set(prefix ${CMAKE_COMMAND} -E env --unset=${NAME})
	else()
		list(APPEND prefix ${NAME}=${env})
	endif()
	execute_process(COMMAND ${prefix} ${CMAKE_COMMAND} -DNAME=${NAME} "-DVALUE=${value}" -P ${SCRIPT}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	if(want STREQUAL "fail")
		if(rc EQUAL 0)
			message(FATAL_ERROR "check-cache-variable-test: env=\"${env}\" cache=\"${value}\" did not fail")
		endif()
		if(NOT out MATCHES "${NAME}[ \n]+is[ \n]+set[ \n]+in[ \n]+the[ \n]+environment,[ \n]+which[ \n]+has[ \n]+no[ \n]+effect: it[ \n]+is[ \n]+a[ \n]+CMake[ \n]+cache[ \n]+variable[ \n]+\\(cmake[ \n]+--preset[ \n]+amiga[ \n]+-D${NAME}=\\.\\.\\.\\)")
			message(FATAL_ERROR "check-cache-variable-test: the message is wrong: ${out}")
		endif()
	elseif(NOT rc EQUAL 0)
		message(FATAL_ERROR "check-cache-variable-test: env=\"${env}\" cache=\"${value}\" failed: ${out}")
	endif()
endfunction()
check(x "" fail)
check(x "y" pass)
check("" "" pass)
check(unset "" pass)
check(unset "y" pass)
message(STATUS "check-cache-variable-test: all checks as expected")

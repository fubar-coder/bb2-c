# basictest - the codec of the Blitz test library's result file
# (basictest.h, spec testing/basic-test-interface), shared like
# shared/tokenizer: every user compiles the files itself with its own
# options - the host tools writer and reader (tools/lib/basictest), this
# directory the writer as a library compiles it.  macros.bas is the macro
# text of the test drivers; libs/userlibs/testlib copies it next to the
# test library in the step that checks its macro names against the tokens
# (check-macros.cmake).  The test library and the runner that use them
# are the changes basic-testlib and basic-test-runner.
#
# basictest_writer_blitzlib is the writer compiled in the environment of a
# Blitz library (cmake/BlitzLibrary.cmake: the variant's and the shadow
# headers, BLITZLIB_OPTIONS_BEFORE, the size profile, BLITZLIB_OPTIONS_AFTER),
# the form the test library is to link.  In its build step (POST_BUILD) the
# archive must not refer to a symbol it does not define (check-writer.cmake):
# a library has no libc and no libgcc, so a division, a 32-bit multiply or
# a memcpy the compiler emits in the writer fails the build.  ctest basictest_writer_check_broken: a writer that divides;
# basictest_writer_check_good: the check on the archive itself.
set(BASICTEST_CHECK ${CMAKE_CURRENT_SOURCE_DIR}/check-writer.cmake)

function(basictest_blitzlib_object name)
	add_library(${name} STATIC ${ARGN})
	target_include_directories(${name} BEFORE PRIVATE ${BLITZ_DEFAULT_VARIANT_DIR}
		${BLITZLIB_NDK_SHADOW})
	target_include_directories(${name} PRIVATE ${BLITZLIB_INCLUDE} ${CMAKE_CURRENT_SOURCE_DIR})
	target_compile_options(${name} PRIVATE ${BLITZLIB_OPTIONS_BEFORE}
		${BLITZLIB_OPTIMIZE_size} ${BLITZLIB_OPTIONS_AFTER} -Wextra)
	set_target_properties(${name} PROPERTIES C_STANDARD 99 C_EXTENSIONS OFF)
endfunction()

basictest_blitzlib_object(basictest_writer_blitzlib basictest_write.c)
add_custom_command(TARGET basictest_writer_blitzlib POST_BUILD
	COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM} -DLIB=$<TARGET_FILE:basictest_writer_blitzlib>
		-P ${BASICTEST_CHECK}
	VERBATIM)

# the check with deliberately broken input (a writer whose decimal routine
# divides: __udivsi3, __umodsi3) and with the archive above
basictest_blitzlib_object(basictest_writer_divides broken/writer_divides.c)
add_test(NAME basictest_writer_check_broken COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM}
	-DLIB=$<TARGET_FILE:basictest_writer_divides> -P ${BASICTEST_CHECK})
set_tests_properties(basictest_writer_check_broken PROPERTIES
	PASS_REGULAR_EXPRESSION "refers[ \n]+to[ \n]+symbols[ \n]+it[ \n]+does[ \n]+not[ \n]+define[^\n]*\n?[^\n]*__u(div|mod)si3")  # CMake wraps long messages
add_test(NAME basictest_writer_check_good COMMAND ${CMAKE_COMMAND} -DNM=${CMAKE_NM}
	-DLIB=$<TARGET_FILE:basictest_writer_blitzlib> -P ${BASICTEST_CHECK})

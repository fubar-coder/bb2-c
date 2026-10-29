add_executable(bb22 main.c)
target_link_libraries(bb22 PRIVATE lib_bb22)

# the real files: the 9 BB22 sources of rwelibsrc converted and read by
# teddetok (skipped without the Aminet archive or the CD)
get_filename_component(repo ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
add_test(NAME bb22_rwelibsrc COMMAND ${CMAKE_COMMAND} -DBB22=$<TARGET_FILE:bb22>
	-DTEDDETOK=$<TARGET_FILE:teddetok> -DREPO=${repo} -DOUTDIR=${CMAKE_CURRENT_BINARY_DIR}/rwelibsrc
	-P ${CMAKE_CURRENT_SOURCE_DIR}/rwelibsrc.cmake)
set_tests_properties(bb22_rwelibsrc PROPERTIES SKIP_RETURN_CODE 77)

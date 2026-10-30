add_executable(xtra main.c)
target_link_libraries(xtra PRIVATE lib_xtra)

# the tool on files: an AmiBlitz3 header converted to a .xtra and back,
# dumped and checked; broken inputs refused with their message
add_test(NAME xtra_tool COMMAND ${CMAKE_COMMAND} -DXTRA=$<TARGET_FILE:xtra>
	-DWORK=${CMAKE_CURRENT_BINARY_DIR}/tooltest -P ${CMAKE_CURRENT_SOURCE_DIR}/tooltest.cmake)

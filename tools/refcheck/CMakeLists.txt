# refcheck: the port against the originals (README.md)
set(REFCHECK_SUBCOMMANDS asm labels data data-ref listing-map listing-labels listing-verify)
add_executable(refcheck main.cpp cmd_asm.cpp cmd_labels.cpp cmd_data.cpp cmd_listing.cpp)
target_link_libraries(refcheck PRIVATE lib_cli lib_asmcmp lib_biglib lib_checklabels lib_checkdata
	lib_vasmlisting)
set_target_properties(refcheck PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
host_help_test(refcheck ${REFCHECK_SUBCOMMANDS})
host_readme_test(refcheck README.md ${REFCHECK_SUBCOMMANDS})

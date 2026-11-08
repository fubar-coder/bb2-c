# refcheck: the port against the originals (README.md)
set(REFCHECK_SUBCOMMANDS asm labels data listing-map listing-labels listing-verify)
add_executable(refcheck main.c cmd_asm.c cmd_labels.c cmd_data.c cmd_listing.c)
target_link_libraries(refcheck PRIVATE lib_subcmd lib_asmcmp lib_biglib lib_checklabels lib_checkdata
	lib_vasmlisting)
host_help_test(refcheck ${REFCHECK_SUBCOMMANDS})
host_readme_test(refcheck README.md)

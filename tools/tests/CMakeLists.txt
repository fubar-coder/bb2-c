# tests of lib/, with inputs built in memory (testutil.h)
foreach(t hunk blitzlib biglib blitztokens hunkcheck amiga_emulation vasmlisting sha256 checklabels checkdata asmcmp mktestdata)
	add_executable(test_${t} test_${t}.c)
	target_link_libraries(test_${t} PRIVATE lib_${t} lib_hunk)
	target_compile_options(test_${t} PRIVATE -Wno-unused-function)
	add_test(NAME ${t} COMMAND test_${t})
endforeach()

add_executable(test_blitzlib_compare test_blitzlib_compare.c)
target_link_libraries(test_blitzlib_compare PRIVATE lib_blitzlib lib_hunk)
target_compile_options(test_blitzlib_compare PRIVATE -Wno-unused-function)
add_test(NAME blitzlib_compare COMMAND test_blitzlib_compare)

# with the emulator (label emulator; skipped without Amiberry or the
# emulation data in amiga-emulation/): four runs in parallel
add_executable(test_amiga_emulation_run test_amiga_emulation_run.c)
target_link_libraries(test_amiga_emulation_run PRIVATE lib_amiga_emulation lib_hunk)
target_compile_options(test_amiga_emulation_run PRIVATE -Wno-unused-function)
add_test(NAME amiga_emulation_run COMMAND test_amiga_emulation_run)
set_tests_properties(amiga_emulation_run PROPERTIES
	LABELS emulator SKIP_RETURN_CODE 77 TIMEOUT 180)

# The COST of the long tests of the all suite, so that a cold ctest run
# (no Testing/Temporary/CTestCostData.txt yet) starts them first and does not
# end with one of them alone.  ctest sorts by COST in a first run and by its
# measured durations (CTestCostData.txt) in later ones.  The values are the
# rounded durations in seconds of one such cost file (a full run of the all
# suite on a cool machine, tests in parallel); only their order matters.
# Call it at the end of the project's top-level CMakeLists.txt, after all
# add_test.  Tests of this list that do not exist are ignored.
function(long_test_costs)
	set(costs
		ted_ab 280
		ab3_bb2_equivalence_runerr 100
		libs_emulator 85
		ab3_bb2_equivalence 85
		corpus_cd 60
		corpus_cd_ab3 50
		ab3_cli_ref 45
		corpus_req 40
		ab3_tokenise_broken 30
		corpus_serial_runerr 30
		ab3_tokenise 30)
	set(dirs ${CMAKE_SOURCE_DIR})
	while(dirs)
		list(POP_FRONT dirs dir)
		get_property(sub DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
		list(APPEND dirs ${sub})
		get_property(tests DIRECTORY ${dir} PROPERTY TESTS)
		foreach(t IN LISTS tests)
			list(FIND costs ${t} i)
			if(i GREATER_EQUAL 0)
				math(EXPR i "${i} + 1")
				list(GET costs ${i} cost)
				set_property(TEST ${t} DIRECTORY ${dir} PROPERTY COST ${cost})
			endif()
		endforeach()
	endwhile()
endfunction()

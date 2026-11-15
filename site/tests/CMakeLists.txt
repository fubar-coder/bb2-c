# The check of the overview of fixed original bugs
# (site/layouts/_partials/fix-overview/check.html), run by Hugo in podman
# (cmake/site-fixoverview-test.cmake): the site as it is builds cleanly
# (site_fixoverview), and every broken data file of fixoverview/data/ fails
# with its message. Label site; skipped (77) without podman or the image.
# One at a time: they share Hugo's module cache.
set(_site_cache ${CMAKE_BINARY_DIR}/site-hugo-cache)
set(_cases
	"real|"
	"missing-anchor|row #421 names the anchor \"no-such-section\", which is no heading"
	"bad-severity|row #421 has the severity 5, not one of the levels"
	"unknown-topic|row #421 has the topic \"Cooking\", which is not in the topics list"
	"duplicate|issue #421 has more than one row"
	"no-row|issue #999 is linked on [^ ]+ but has no row")
foreach(c IN LISTS _cases)
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 case)
	list(GET c 1 expect)
	if(case STREQUAL "real")
		set(name site_fixoverview)
	else()
		set(name site_fixoverview_${case})
		string(REPLACE "-" "_" name ${name})
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DSITE=${PROJECT_SOURCE_DIR}/site -DCASE=${case} "-DEXPECT=${expect}"
		-DCACHE=${_site_cache} -DWORK=${CMAKE_BINARY_DIR}/${name}
		-DTEST_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/site-fixoverview-test.cmake
		-P ${WITH_WORKDIR})
	set_tests_properties(${name} PROPERTIES LABELS site SKIP_RETURN_CODE 77
		RESOURCE_LOCK site_hugo_cache TIMEOUT 300)
endforeach()

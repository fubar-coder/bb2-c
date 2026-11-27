# The check of the overview of fixed original bugs
# (cmake/site-fixoverview-test.cmake), in both generators until Hugo is
# removed (site-starlight, task 9.2): the site as it is builds cleanly, and
# every broken data file of fixoverview/data/ fails with its message. The
# Starlight build (src/lib/fix-overview.mjs, the data file named by
# BB2_FIXOVERVIEW_DATA; a failed build leaves no dist/): site_fixoverview and
# site_fixoverview_*; Hugo's (layouts/_partials/fix-overview/check.html):
# site_hugo_fixoverview and site_hugo_fixoverview_*, the same cases and
# messages. Label site; Hugo's skipped (77) without podman or the image, the
# Starlight ones as the Starlight build below. One at a time per generator:
# they share npm's cache or Hugo's module cache.
#
# SITE_NODE: where the Starlight builds run Node.js
# (cmake/site-starlight-test.cmake): auto (the host's node when it has the
# version of engines.node in site/package.json, else the pinned image via
# podman, else skipped), host or container (that way only; not available
# fails the test).
set(SITE_NODE auto CACHE STRING
	"Where the site tests run Node.js: auto, host (node at the pinned version) or container (podman)")
set_property(CACHE SITE_NODE PROPERTY STRINGS auto host container)
if(NOT SITE_NODE MATCHES "^(auto|host|container)$")
	message(FATAL_ERROR "SITE_NODE is auto, host or container, not ${SITE_NODE}")
endif()
set(_site_cache ${CMAKE_BINARY_DIR}/site-hugo-cache)
set(_starlight_cache ${CMAKE_BINARY_DIR}/site-npm-cache)
set(_cases
	"real|"
	"missing-anchor|row #421 names the anchor \"no-such-section\", which is no heading"
	"no-short|row #421 has no short"
	"bad-levels|the levels are 1,2,3,4, not the five levels 1 to 5"
	"bad-severity|row #421 has the severity 6, not one of the levels"
	"unknown-topic|row #421 has the topic \"Cooking\", which is not in the topics list"
	"bad-highlight|row #421 has the highlight \"yes\", not true or false"
	"duplicate|issue #421 has more than one row"
	"no-row|issue #999 is linked on [^ ]+ but has no row")
foreach(generator starlight hugo)
	if(generator STREQUAL "starlight")
		set(prefix site_fixoverview)
		set(cache ${_starlight_cache})
		set(lock site_npm_cache)
		set(timeout 600)
	else()
		set(prefix site_hugo_fixoverview)
		set(cache ${_site_cache})
		set(lock site_hugo_cache)
		set(timeout 300)
	endif()
	foreach(c IN LISTS _cases)
		string(REPLACE "|" ";" c "${c}")
		list(GET c 0 case)
		list(GET c 1 expect)
		if(case STREQUAL "real")
			set(name ${prefix})
		else()
			set(name ${prefix}_${case})
			string(REPLACE "-" "_" name ${name})
		endif()
		add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
			-DSITE=${PROJECT_SOURCE_DIR}/site -DGENERATOR=${generator} -DCASE=${case}
			"-DEXPECT=${expect}" -DSITE_NODE=${SITE_NODE}
			-DCACHE=${cache} -DWORK=${CMAKE_BINARY_DIR}/${name}
			-DTEST_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/site-fixoverview-test.cmake
			-P ${WITH_WORKDIR})
		set_tests_properties(${name} PROPERTIES LABELS site SKIP_RETURN_CODE 77
			RESOURCE_LOCK ${lock} TIMEOUT ${timeout})
	endforeach()
endforeach()

# The Starlight build (cmake/site-starlight-test.cmake), Node.js at the
# pinned version (on the host or in the pinned image via podman, SITE_NODE
# above), in a copy of site/: the site as it is installs and
# builds without a warning (site_starlight); the fixture starlight/warning/
# (two pages with one slug) makes a warning that fails the check
# (site_starlight_warning); the fixtures of the build's checks
# (src/integrations/site-checks.mjs) fail the build with their message: a
# page linking a missing page (site_links_missing_page) or a missing heading
# id (site_links_missing_anchor), a data file linking a missing page
# (site_links_data), a data file with a relref while the leftover-syntax
# check is on (site_hugo_syntax), an anchorMoves naming a missing page
# (site_alias_bad_move), a list of old URLs (site/tests/urls/) with a URL
# the build does not write (site_urls_missing). Label site; skipped (77) when
# neither way is available (SITE_NODE auto) or when the dependencies cannot
# be downloaded. One at a time: they share npm's cache.
set(_starlight_cases
	"site_starlight|real|||"
	"site_starlight_warning|warning|warning|multiple entries with the same slug|"
	"site_links_missing_page|links-missing-page|error|page /bb2-c/fixture-links/: the link /bb2-c/docs/no-such-page/ leads to no page|"
	"site_links_missing_anchor|links-missing-anchor|error|page /bb2-c/fixture-links/: the link /bb2-c/docs/libraries/#no-such-heading names the id \"no-such-heading\"|"
	"site_links_data|links-data|error|data/cmddesc/acidlibs/fixture-links.yaml \\(Fixture.notes\\): the link /docs/no-such-page/ leads to no page|"
	"site_hugo_syntax|hugo-syntax|error|data/cmddesc/acidlibs/fixture-hugo-syntax.yaml:7: \\{\\{< relref|BB2_HUGO_SYNTAX_CHECK=1"
	"site_alias_bad_move|alias-bad-move|error|page /bb2-c/fixture-alias/: anchorMoves names the page \"no-such-page\"|"
	"site_urls_missing|urls-missing|error|tests/urls/fixture.txt:4: the old URL /docs/no-such-old-page/ is not in the output|")
foreach(c IN LISTS _starlight_cases)
	string(REPLACE "|" ";" c "${c}")
	list(GET c 0 name)
	list(GET c 1 case)
	list(GET c 2 kind)
	list(GET c 3 expect)
	list(GET c 4 env)
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DSITE=${PROJECT_SOURCE_DIR}/site -DCASE=${case} "-DEXPECT=${expect}"
		-DKIND=${kind} -DENV=${env} -DSITE_NODE=${SITE_NODE}
		-DCACHE=${_starlight_cache} -DWORK=${CMAKE_BINARY_DIR}/${name}
		-DTEST_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/site-starlight-test.cmake
		-P ${WITH_WORKDIR})
	set_tests_properties(${name} PROPERTIES LABELS site SKIP_RETURN_CODE 77
		RESOURCE_LOCK site_npm_cache TIMEOUT 600)
endforeach()

# The Doxygen run of the Pages workflow, out of tree
# (cmake/api-doxygen-test.cmake): the comments of the libraries give no
# warning (api_doxygen), and a broken comment fails the run
# (api_doxygen_broken). Label site; skipped (77) without podman or the image.
foreach(case real broken)
	if(case STREQUAL "real")
		set(name api_doxygen)
	else()
		set(name api_doxygen_broken)
	endif()
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND}
		-DSOURCE=${PROJECT_SOURCE_DIR} -DCASE=${case} -DWORK=${CMAKE_BINARY_DIR}/${name}
		-DTEST_SCRIPT=${PROJECT_SOURCE_DIR}/cmake/api-doxygen-test.cmake
		-P ${WITH_WORKDIR})
	set_tests_properties(${name} PROPERTIES LABELS site SKIP_RETURN_CODE 77 TIMEOUT 600)
endforeach()

# site-fixoverview-test.cmake - builds the website to test the check of the
# overview of fixed original bugs (site/src/lib/fix-overview.mjs):
#
#   cmake -DSITE=<repo>/site -DCASE=real|<broken case>
#         [-DEXPECT=regex] [-DSITE_NODE=auto|host|container]
#         -DCACHE=<dir> -DWORK=<dir> -P site-fixoverview-test.cmake
#
# CASE real: the site as it is must build without errors or warnings.
# Another CASE names a broken data file site/tests/fixoverview/data/CASE.yaml;
# the build must fail with output matching EXPECT and leave no dist/.
#
# The build is cmake/site-starlight-test.cmake (CACHE npm's cache,
# SITE_NODE where Node.js runs); the data file is given by the environment
# variable BB2_FIXOVERVIEW_DATA (data/fixoverview.yaml for CASE real, so
# that the variable's path is tested too). Exits 77 (skipped) as that
# script does.
foreach(v SITE CASE CACHE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "site-fixoverview-test: ${v} is needed")
	endif()
endforeach()

if(CASE STREQUAL "real")
	set(ENV BB2_FIXOVERVIEW_DATA=data/fixoverview.yaml)
	set(KIND "")
else()
	if(NOT EXISTS ${SITE}/tests/fixoverview/data/${CASE}.yaml)
		message(FATAL_ERROR "site-fixoverview-test: no data file ${SITE}/tests/fixoverview/data/${CASE}.yaml")
	endif()
	set(ENV BB2_FIXOVERVIEW_DATA=tests/fixoverview/data/${CASE}.yaml)
	set(KIND error)
endif()
set(CASE real)
include(${CMAKE_CURRENT_LIST_DIR}/site-starlight-test.cmake)

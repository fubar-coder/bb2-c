# site-fixoverview-test.cmake - builds the website to test the check of the
# overview of fixed original bugs:
#
#   cmake -DSITE=<repo>/site -DGENERATOR=starlight|hugo -DCASE=real|<broken case>
#         [-DEXPECT=regex] -DCACHE=<dir> -DWORK=<dir> -P site-fixoverview-test.cmake
#
# CASE real: the site as it is must build without errors or warnings.
# Another CASE names a broken data file site/tests/fixoverview/data/CASE.yaml;
# the build must fail with output matching EXPECT.
#
# GENERATOR starlight: the Starlight build (cmake/site-starlight-test.cmake,
# CACHE npm's cache), the check src/lib/fix-overview.mjs; the data file is
# given by the environment variable BB2_FIXOVERVIEW_DATA (data/fixoverview.yaml
# for CASE real), and a failed build must leave no dist/.
# GENERATOR hugo: Hugo in podman (the image of site/README.md), the check
# layouts/_partials/fix-overview/check.html; the data file is given by the
# site parameter fixOverviewData (tests/fixoverview/config.yaml), CACHE keeps
# Hugo's module cache between runs (the theme). Hugo's tests stay until Hugo
# is removed (site-starlight, task 9.2).
#
# Exits 77 (skipped) without podman, without the image or when the theme or
# the dependencies cannot be downloaded.
foreach(v SITE GENERATOR CASE CACHE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "site-fixoverview-test: ${v} is needed")
	endif()
endforeach()

if(GENERATOR STREQUAL "starlight")
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
	return()
elseif(NOT GENERATOR STREQUAL "hugo")
	message(FATAL_ERROR "site-fixoverview-test: GENERATOR is starlight or hugo, not ${GENERATOR}")
endif()

set(IMAGE docker.io/hugomods/hugo:go-git-0.165.0)
find_program(PODMAN podman)
if(NOT PODMAN)
	message(STATUS "skipped: no podman")
	cmake_language(EXIT 77)
endif()
execute_process(COMMAND ${PODMAN} image exists ${IMAGE} RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
	message(STATUS "skipped: no image ${IMAGE} (podman pull ${IMAGE})")
	cmake_language(EXIT 77)
endif()
file(MAKE_DIRECTORY ${WORK} ${CACHE})
set(args)
if(NOT CASE STREQUAL "real")
	set(args -e HUGO_PARAMS_FIXOVERVIEWDATA=fixtests.${CASE})
	set(config --config hugo.yaml,tests/fixoverview/config.yaml)
endif()
execute_process(COMMAND ${PODMAN} run --rm
		-v ${SITE}:/src -v ${WORK}:/out -v ${CACHE}:/cache
		-e HUGO_CACHEDIR=/cache ${args} -w /src ${IMAGE}
		hugo --gc --minify --enableGitInfo=false --noBuildLock ${config}
		--baseURL https://fubar-coder.github.io/bb2-c/ -d /out/public
	RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
if(out MATCHES "failed to download|dial tcp|no such host")
	message(STATUS "skipped: the theme cannot be downloaded\n${out}")
	cmake_language(EXIT 77)
endif()
if(CASE STREQUAL "real")
	if(NOT rc EQUAL 0 OR out MATCHES "(ERROR|WARN)")
		message(FATAL_ERROR "the site does not build cleanly (${rc}):\n${out}")
	endif()
elseif(rc EQUAL 0)
	message(FATAL_ERROR "the broken data file ${CASE} built:\n${out}")
elseif(NOT out MATCHES "${EXPECT}")
	message(FATAL_ERROR "the broken data file ${CASE} failed without the message \"${EXPECT}\":\n${out}")
else()
	message(STATUS "${CASE}: failed as expected")
endif()

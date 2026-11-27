# site-starlight-test.cmake - builds the Starlight website (site/package.json,
# site/astro.config.mjs) with Node.js in the pinned image via podman, in a
# copy of site/ in the work directory:
#
#   cmake -DSITE=<repo>/site -DCASE=real|<fixture> [-DEXPECT=regex]
#         [-DKIND=warning|error] [-DENV=VAR=value...]
#         -DCACHE=<dir> -DWORK=<dir> -P site-starlight-test.cmake
#
# The copy leaves out what the builds write (node_modules/, dist/, .astro/,
# Hugo's public/ and resources/); the dependencies are installed into it
# with `npm ci` from the lock file, CACHE keeping npm's download cache
# between runs. CASE real: the site as it is must install and build with
# exit status 0 and without a warning line. Another CASE names a fixture
# site/tests/starlight/CASE/, whose files are copied over the copy of the
# site; the build must then fail: KIND warning (the default) on a warning
# line matching EXPECT; KIND error (a check of the build) with a non-zero
# exit status, an output matching EXPECT and no dist/ left behind. KIND
# error also takes CASE real: the site as it is, broken by ENV alone. ENV:
# the environment variables of the build (a list of VAR=value).
# Exits 77 (skipped) without podman, without the image or when the
# dependencies cannot be downloaded.
set(IMAGE docker.io/library/node:26.13.1-bookworm-slim)
foreach(v SITE CASE CACHE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "site-starlight-test: ${v} is needed")
	endif()
endforeach()
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
file(COPY ${SITE}/ DESTINATION ${WORK}/site
	REGEX "^${SITE}/(node_modules|dist|\\.astro|public|resources|\\.hugo_build\\.lock)$" EXCLUDE)
if(NOT CASE STREQUAL "real")
	if(NOT IS_DIRECTORY ${SITE}/tests/starlight/${CASE})
		message(FATAL_ERROR "site-starlight-test: no fixture ${SITE}/tests/starlight/${CASE}")
	endif()
	file(COPY ${SITE}/tests/starlight/${CASE}/ DESTINATION ${WORK}/site)
endif()

# a warning line of npm, Astro, Vite/Rollup or Node.js
set(warning_line "(^|\n)[^\n]*(npm warn|\\[WARN\\]|\\(!\\)|[Ww]arning)[^\n]*")
if(NOT KIND)
	set(KIND warning)
endif()
set(env_args)
foreach(e IN LISTS ENV)
	list(APPEND env_args -e ${e})
endforeach()
function(run what)
	execute_process(COMMAND ${PODMAN} run --rm
			-v ${WORK}/site:/site -v ${CACHE}:/npm-cache -e npm_config_cache=/npm-cache
			${env_args} -w /site ${IMAGE} ${ARGN}
		RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	set(rc ${rc} PARENT_SCOPE)
	set(out "${out}" PARENT_SCOPE)
endfunction()

run(install npm ci --prefer-offline)
if(NOT rc EQUAL 0 AND out MATCHES "ENOTFOUND|EAI_AGAIN|ECONNREFUSED|ECONNRESET|ETIMEDOUT|network")
	message(STATUS "skipped: the dependencies cannot be downloaded\n${out}")
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL 0 OR out MATCHES "${warning_line}")
	message(FATAL_ERROR "npm ci does not install cleanly (${rc}):\n${out}")
endif()

run(build npm run build)
string(REGEX MATCH "${warning_line}" warning "${out}")
if(KIND STREQUAL "error")
	if(CASE STREQUAL "real")
		set(what "the build with ${ENV}")
	else()
		set(what "the fixture ${CASE}")
	endif()
	if(rc EQUAL 0)
		message(FATAL_ERROR "${what} built without an error:\n${out}")
	elseif(NOT out MATCHES "${EXPECT}")
		message(FATAL_ERROR "${what} failed without \"${EXPECT}\" (${rc}):\n${out}")
	elseif(EXISTS ${WORK}/site/dist)
		message(FATAL_ERROR "${what} failed, but left ${WORK}/site/dist:\n${out}")
	endif()
	string(REGEX MATCH "[^\n]*${EXPECT}[^\n]*" line "${out}")
	message(STATUS "${what}: the build fails as expected: ${line}")
elseif(CASE STREQUAL "real")
	if(NOT rc EQUAL 0 OR warning)
		message(FATAL_ERROR "the site does not build cleanly (${rc}):\n${out}")
	endif()
elseif(NOT warning)
	message(FATAL_ERROR "the fixture ${CASE} built without a warning (${rc}):\n${out}")
elseif(NOT warning MATCHES "${EXPECT}")
	message(FATAL_ERROR "the fixture ${CASE} warned without \"${EXPECT}\":\n${out}")
else()
	message(STATUS "${CASE}: the warning fails the build check as expected:${warning}")
endif()

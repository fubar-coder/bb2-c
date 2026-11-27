# site-starlight-test.cmake - builds the Starlight website (site/package.json,
# site/astro.config.mjs) with Node.js at the pinned version, in a copy of
# site/ in the work directory:
#
#   cmake -DSITE=<repo>/site -DCASE=real|<fixture> [-DEXPECT=regex]
#         [-DKIND=warning|error] [-DENV=VAR=value...]
#         [-DSITE_NODE=auto|host|container]
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
#
# SITE_NODE, where Node.js runs: host, the host's node and npm, whose
# `node --version` must be the version of `engines.node` in
# site/package.json; container, the pinned image via podman; auto (the
# default), the host when its node has the pinned version, else the
# container. A host or container named explicitly that is not available
# fails the test; auto exits 77 (skipped) when neither is available, and
# every way exits 77 when the dependencies cannot be downloaded.
set(IMAGE docker.io/library/node:26.13.1-bookworm-slim)
foreach(v SITE CASE CACHE WORK)
	if(NOT ${v})
		message(FATAL_ERROR "site-starlight-test: ${v} is needed")
	endif()
endforeach()
if(NOT SITE_NODE)
	set(SITE_NODE auto)
endif()
if(NOT SITE_NODE MATCHES "^(auto|host|container)$")
	message(FATAL_ERROR "site-starlight-test: SITE_NODE is auto, host or container, not ${SITE_NODE}")
endif()

# the host's node, when it has the pinned version
file(READ ${SITE}/package.json package)
string(JSON pinned GET "${package}" engines node)
set(host_why "")
find_program(NODE node)
find_program(NPM npm)
if(NOT NODE OR NOT NPM)
	set(host_why "no node and npm on the host")
else()
	execute_process(COMMAND ${NODE} --version RESULT_VARIABLE rc
		OUTPUT_VARIABLE node_version ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT rc EQUAL 0 OR NOT node_version STREQUAL "v${pinned}")
		set(host_why "the host's ${NODE} is ${node_version}, not the pinned v${pinned}")
	endif()
endif()
# the pinned image via podman
set(container_why "")
find_program(PODMAN podman)
if(NOT PODMAN)
	set(container_why "no podman")
else()
	execute_process(COMMAND ${PODMAN} image exists ${IMAGE} RESULT_VARIABLE rc)
	if(NOT rc EQUAL 0)
		set(container_why "no image ${IMAGE} (podman pull ${IMAGE})")
	endif()
endif()
if(SITE_NODE STREQUAL "auto")
	if(NOT host_why)
		set(SITE_NODE host)
	elseif(NOT container_why)
		set(SITE_NODE container)
	else()
		message(STATUS "skipped: ${host_why}; ${container_why}")
		cmake_language(EXIT 77)
	endif()
elseif(${SITE_NODE}_why)
	message(FATAL_ERROR "site-starlight-test: SITE_NODE=${SITE_NODE}, but ${${SITE_NODE}_why}")
endif()
if(SITE_NODE STREQUAL "host")
	message(STATUS "Node.js: the host's ${NODE} ${node_version}")
else()
	message(STATUS "Node.js: ${IMAGE} via podman")
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
function(run what)
	if(SITE_NODE STREQUAL "host")
		execute_process(COMMAND ${CMAKE_COMMAND} -E env npm_config_cache=${CACHE} ${ENV}
				${NPM} ${ARGN}
			WORKING_DIRECTORY ${WORK}/site
			RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	else()
		set(env_args)
		foreach(e IN LISTS ENV)
			list(APPEND env_args -e ${e})
		endforeach()
		execute_process(COMMAND ${PODMAN} run --rm
				-v ${WORK}/site:/site -v ${CACHE}:/npm-cache -e npm_config_cache=/npm-cache
				${env_args} -w /site ${IMAGE} npm ${ARGN}
			RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE out)
	endif()
	set(rc ${rc} PARENT_SCOPE)
	set(out "${out}" PARENT_SCOPE)
endfunction()

run(install ci --prefer-offline)
if(NOT rc EQUAL 0 AND out MATCHES "ENOTFOUND|EAI_AGAIN|ECONNREFUSED|ECONNRESET|ETIMEDOUT|network")
	message(STATUS "skipped: the dependencies cannot be downloaded\n${out}")
	cmake_language(EXIT 77)
endif()
if(NOT rc EQUAL 0 OR out MATCHES "${warning_line}")
	message(FATAL_ERROR "npm ci does not install cleanly (${rc}):\n${out}")
endif()

run(build run build)
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

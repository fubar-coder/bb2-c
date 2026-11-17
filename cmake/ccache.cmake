# ccache for the C and C++ compilations of the build (docs/development.md,
# "ccache"): the m68k side (libraries, blitz2, TED), the host side (the
# ExternalProject host_tools) and the prev build (cmake/prev.cmake).
#
#   -DBB2C_CCACHE=ON|OFF   default ON if a ccache 4.14 or later is found
#
# ccache 4.14 or later: its NEWS ("Ccache 4.14": "Added detection of C23
# #embed directives in direct and depend mode ..., preventing false cache
# hits") - an older ccache could take a result of the direct mode although
# a file read by #embed changed.  With #embed the direct mode is not used,
# the preprocessor mode hashes the output of `gcc -E` (manual, "The
# preprocessor mode"), which holds the embedded bytes (GCC's cpp manual,
# "Binary Resource Inclusion").  The tests ccache_embed (m68k) and
# ccache_embed_host (host/) check it with a changed embedded file.
#
# The settings are ccache's command line settings KEY=VALUE (manual,
# "Configuration": they come before the environment and every
# configuration file), so the user's ccache.conf cannot weaken them; the
# cache directory and its size stay the user's (default ~/.cache/ccache):
# - sloppiness= (none), depend_mode=false, run_second_cpp=true,
#   compiler_check=mtime: the documented defaults, set so that no
#   configuration of the user changes what counts as the same compilation
# - base_dir=DIRS (only without debug information): absolute paths below
#   the source and the build directory are made relative to the working
#   directory before the compiler sees them (manual, "base_dir"), so that
#   another checkout (a git worktree) at another path shares the cache.
#   It changes what the compiler is given: __FILE__ and the paths in
#   debug information become relative.  The results have neither (no -g,
#   __FILE__ only in host code: ORIG_HAZARD of TED_HOST, the snitch
#   messages), so they stay the same (docs/development.md, "ccache");
#   with -g (CMAKE_BUILD_TYPE Debug, BLITZ2_COVERAGE) or HOST_COVERAGE
#   (-fprofile-filter-files matches absolute paths) there is no base_dir.
#   absolute_paths_in_stderr=true: warnings and errors name the files
#   with absolute paths again
#
# Sets CMAKE_C_COMPILER_LAUNCHER and CMAKE_CXX_COMPILER_LAUNCHER, unless
# the caller gave them (-D or the environment; the prev build gets its
# own so), BB2C_HOST_LAUNCHER (the launcher of the host side) and the
# function bb2c_ccache_launcher(OUT DIRS DEBUG).

set(_ccache_default OFF)
find_program(BB2C_CCACHE_PROGRAM ccache DOC "ccache (BB2C_CCACHE)")
set(BB2C_CCACHE_VERSION "")
if(BB2C_CCACHE_PROGRAM)
	execute_process(COMMAND ${BB2C_CCACHE_PROGRAM} --version
		OUTPUT_VARIABLE _ccache_out ERROR_QUIET RESULT_VARIABLE _ccache_rc)
	if(NOT _ccache_rc AND _ccache_out MATCHES "ccache version ([0-9]+(\\.[0-9]+)+)")
		set(BB2C_CCACHE_VERSION ${CMAKE_MATCH_1})
		if(BB2C_CCACHE_VERSION VERSION_GREATER_EQUAL 4.14)
			set(_ccache_default ON)
		endif()
	endif()
endif()
option(BB2C_CCACHE "compile with ccache (4.14 or later)" ${_ccache_default})

# bb2c_ccache_launcher(OUT DIRS DEBUG): the ccache command line of a build
# whose sources and build directory are below DIRS (a list); DEBUG true:
# the build has debug information or coverage (no base_dir)
function(bb2c_ccache_launcher out dirs debug)
	set(l ${BB2C_CCACHE_PROGRAM} sloppiness= depend_mode=false run_second_cpp=true
		compiler_check=mtime)
	if(NOT debug)
		list(JOIN dirs ":" d)
		list(APPEND l base_dir=${d} absolute_paths_in_stderr=true)
	endif()
	set(${out} "${l}" PARENT_SCOPE)
endfunction()

set(BB2C_CCACHE_OWN OFF)       # the launcher is this file's (else the caller's or none)
set(BB2C_CCACHE_DEBUG OFF)     # this build (not the host side) has debug information or coverage
string(TOUPPER "${CMAKE_BUILD_TYPE}" _ccache_config)
if(BLITZ2_COVERAGE OR (BLITZ2_HOST AND HOST_COVERAGE)
		OR " ${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_${_ccache_config}} " MATCHES " -g")
	set(BB2C_CCACHE_DEBUG ON)
endif()
# the directories of base_dir: the source directory, and the build
# directory if it is not below it
set(BB2C_CCACHE_DIRS ${PROJECT_SOURCE_DIR})
cmake_path(IS_PREFIX PROJECT_SOURCE_DIR ${CMAKE_BINARY_DIR} NORMALIZE _ccache_inside)
if(NOT _ccache_inside)
	list(APPEND BB2C_CCACHE_DIRS ${CMAKE_BINARY_DIR})
endif()

if(DEFINED CMAKE_C_COMPILER_LAUNCHER OR DEFINED ENV{CMAKE_C_COMPILER_LAUNCHER})
	if(NOT DEFINED CMAKE_C_COMPILER_LAUNCHER)
		set(CMAKE_C_COMPILER_LAUNCHER $ENV{CMAKE_C_COMPILER_LAUNCHER})
	endif()
	if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CMAKE_C_COMPILER_LAUNCHER})
	endif()
	set(BB2C_HOST_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}")
	message(STATUS "ccache: the given compiler launcher \"${CMAKE_C_COMPILER_LAUNCHER}\"")
elseif(BB2C_CCACHE)
	if(NOT BB2C_CCACHE_PROGRAM OR NOT BB2C_CCACHE_VERSION OR BB2C_CCACHE_VERSION VERSION_LESS 4.14)
		message(FATAL_ERROR "BB2C_CCACHE=ON needs ccache 4.14 or later (found: "
			"\"${BB2C_CCACHE_PROGRAM}\" ${BB2C_CCACHE_VERSION}); -DBB2C_CCACHE=OFF builds without")
	endif()
	set(BB2C_CCACHE_OWN ON)
	bb2c_ccache_launcher(CMAKE_C_COMPILER_LAUNCHER "${BB2C_CCACHE_DIRS}" ${BB2C_CCACHE_DEBUG})
	set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_C_COMPILER_LAUNCHER}")
	bb2c_ccache_launcher(BB2C_HOST_LAUNCHER "${BB2C_CCACHE_DIRS}" "${HOST_COVERAGE}")
	execute_process(COMMAND ${BB2C_CCACHE_PROGRAM} --get-config cache_dir
		OUTPUT_VARIABLE _ccache_dir OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
	message(STATUS "ccache ${BB2C_CCACHE_VERSION}: ${BB2C_CCACHE_PROGRAM}, cache ${_ccache_dir}")
else()
	set(BB2C_HOST_LAUNCHER "")
	message(STATUS "ccache: not used (BB2C_CCACHE=OFF)")
endif()

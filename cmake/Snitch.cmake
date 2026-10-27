# snitch (C++ test framework, https://github.com/snitch-org/snitch), fixed
# version.  A copy that ab3c already downloaded is used if there is one (no
# network needed); otherwise FetchContent downloads it.
include(FetchContent)
foreach(_dir amiga-release host-debug amiga-hot)
	set(_snitch_local ${CMAKE_CURRENT_LIST_DIR}/../../ab3c/build/${_dir}/_deps/snitch-src)
	if(NOT FETCHCONTENT_SOURCE_DIR_SNITCH AND EXISTS ${_snitch_local}/CMakeLists.txt)
		get_filename_component(_snitch_local ${_snitch_local} ABSOLUTE)
		set(FETCHCONTENT_SOURCE_DIR_SNITCH ${_snitch_local} CACHE PATH "")
	endif()
endforeach()
FetchContent_Declare(snitch
	URL https://github.com/snitch-org/snitch/archive/refs/tags/v1.3.2.tar.gz
	URL_HASH SHA256=5b3a0af7e5637f02ac95652f2238b89eb227b4881bf4e5ffc6935f184ff11626
	EXCLUDE_FROM_ALL
	SYSTEM)
FetchContent_MakeAvailable(snitch)

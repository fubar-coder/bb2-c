# snitch (C++ test framework, https://github.com/snitch-org/snitch), fixed
# version.  A source given as FETCHCONTENT_SOURCE_DIR_SNITCH is used (the
# top-level build gives its own to the host side and to a prev build,
# cmake/prev-build.cmake); otherwise FetchContent downloads it into the
# build's _deps.
include(FetchContent)
FetchContent_Declare(snitch
	URL https://github.com/snitch-org/snitch/archive/refs/tags/v1.3.2.tar.gz
	URL_HASH SHA256=5b3a0af7e5637f02ac95652f2238b89eb227b4881bf4e5ffc6935f184ff11626
	EXCLUDE_FROM_ALL
	SYSTEM)
FetchContent_MakeAvailable(snitch)

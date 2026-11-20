# check-cache-variable.cmake - the first command of a target that is
# steered by a CMake cache variable (libs-capture, libs-update:
# LIBS_CAPTURE_FILTER, LIBS_UPDATE_FIX, LIBS_MACHINE, LIBS_UPDATE_NEW;
# bug-orig-record: BUG_ORIG_RECORD_FILTER): fails when the environment variable of that
# name is set and the cache variable (VALUE, as configured) is empty.
# Such a variable has no effect in the environment, so the target would
# silently run unfiltered.  Both set: the cache variable wins, no message.
#   cmake -DNAME=VAR -DVALUE=CACHE-VALUE -P check-cache-variable.cmake
if(NOT NAME)
	message(FATAL_ERROR "check-cache-variable.cmake: -DNAME=... missing")
endif()
if(NOT "$ENV{${NAME}}" STREQUAL "" AND "${VALUE}" STREQUAL "")
	message(FATAL_ERROR "${NAME} is set in the environment, which has no effect: it is a CMake cache variable (cmake --preset amiga -D${NAME}=...)")
endif()

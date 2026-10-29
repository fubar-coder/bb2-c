# Toolchain file for the m68k-amigaos-gcc (bebbo / AmigaDev), the only one of
# the project; the top-level CMakeLists.txt uses it by default.
#   cmake --preset amiga
# AMIGA_TOOLCHAIN_PREFIX (-D or environment) defaults to the installation
# of the m68k-amigaos-gcc found in PATH, else the first of /opt/amiga,
# /opt/m68k-amigaos that has it (an IDE often starts CMake without the
# shell's PATH).

set(CMAKE_SYSTEM_NAME Generic)         # CMake has no AmigaOS platform module
set(AMIGAOS ON)
set(CMAKE_SYSTEM_PROCESSOR m68k)

if(NOT AMIGA_TOOLCHAIN_PREFIX AND DEFINED ENV{AMIGA_TOOLCHAIN_PREFIX})
	set(AMIGA_TOOLCHAIN_PREFIX $ENV{AMIGA_TOOLCHAIN_PREFIX})
endif()
if(NOT AMIGA_TOOLCHAIN_PREFIX)
	find_program(_amiga_gcc m68k-amigaos-gcc NO_CACHE)
	if(_amiga_gcc)
		get_filename_component(_amiga_gcc ${_amiga_gcc} REALPATH)
		get_filename_component(_amiga_bin ${_amiga_gcc} DIRECTORY)
		get_filename_component(AMIGA_TOOLCHAIN_PREFIX ${_amiga_bin} DIRECTORY)
	else()
		set(AMIGA_TOOLCHAIN_PREFIX /opt/amiga)
		foreach(_p /opt/amiga /opt/m68k-amigaos)
			if(EXISTS ${_p}/bin/m68k-amigaos-gcc)
				set(AMIGA_TOOLCHAIN_PREFIX ${_p})
				break()
			endif()
		endforeach()
	endif()
endif()
# Blitz 2 and TED ran on a plain 68000; the generated code and the glue
# assume that.
set(AMIGA_CPU "68000" CACHE STRING "Target CPU (-m<cpu>)")

set(CMAKE_C_COMPILER   ${AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-gcc)
set(CMAKE_CXX_COMPILER ${AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-g++)
set(CMAKE_AR         ${AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-gcc-ar)
set(CMAKE_RANLIB     ${AMIGA_TOOLCHAIN_PREFIX}/bin/m68k-amigaos-gcc-ranlib)
set(VASM             ${AMIGA_TOOLCHAIN_PREFIX}/bin/vasmm68k_mot)

set(CMAKE_C_FLAGS_INIT          "-m${AMIGA_CPU} -noixemul")
set(CMAKE_CXX_FLAGS_INIT        "-m${AMIGA_CPU} -noixemul")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-m${AMIGA_CPU} -noixemul")
# Not *_RELEASE_INIT: CMake would append its default "-O3 -DNDEBUG".
set(CMAKE_C_FLAGS_RELEASE "-Os" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "-Os" CACHE STRING "")

set(CMAKE_FIND_ROOT_PATH ${AMIGA_TOOLCHAIN_PREFIX}/m68k-amigaos)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

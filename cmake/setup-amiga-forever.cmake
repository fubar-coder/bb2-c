# Set up the emulation data of the emulator tests from Amiga Forever 11
# (Cloanto; the user's own copy): copy the Kickstart ROMs and rom.key, copy
# and adapt the Workbench hardfiles (AMIGA-EMULATION-SETUP.md) - with
# CMake and hdftool (tools/hdftool) only.
#
#   cmake -DAMIGA_FOREVER=DIR [-DHDFTOOL=PROGRAM] [-DDEST=DIR] [-DFORCE=ON]
#         -P cmake/setup-amiga-forever.cmake
#
#   AMIGA_FOREVER  the Amiga Forever medium or installation: a directory
#                  with "Amiga Files/Shared", "Shared" or directly rom/
#                  and hdf/ (e.g. /run/media/$USER/AmigaForever)
#   HDFTOOL        default build/amiga/host/tools/hdftool/hdftool (built
#                  with the host side: cmake --build --preset amiga)
#   DEST           default amiga-emulation/ of the repository
#   FORCE          replace files in DEST that differ from the result (off:
#                  such a file stops the script, nothing is overwritten)
#
# Every input must have the expected SHA-256 (Amiga Forever 11; another
# edition is refused), and so must every result: hdftool works
# reproducibly, the same inputs give the same hardfiles.  A result is
# written as NAME.part beside the target and renamed when it is right.
# Files in DEST that already have the expected contents stay untouched.
#
# Results (the a500 machine's configuration, a500.uae, uses the 1.3 ones;
# the 3.1 ones are for the a1200 and a600, which still use AmigaOS 3.2,
# TODO.md):
#   amiga-os-130.rom        Kickstart 1.3 (34.5), encrypted
#   rom.key                 Amiga Forever's key of the encrypted ROMs
#   workbench-135.hdf       Workbench 1.3.5: RDB hardfile (FFS in the RDB),
#                           S/User-Startup added, S/Startup-Sequence
#                           executes it
#   amiga-os-310-a1200.rom  Kickstart 3.1 (40.68) A1200, encrypted
#   amiga-os-310-a600.rom   Kickstart 3.1 (40.63) A500/A600/A2000, encrypted
#   workbench-311.hdf       Workbench 3.1: the partition of Amiga Forever's
#                           RDB hardfile as a raw image (DOS\3, 32 sectors,
#                           1 surface, 2 reserved blocks), S/User-Startup
#                           added
#   workbench-311-rtg.hdf   the same with the uaegfx monitor
#                           (Storage/Monitors/UAE) in Devs/Monitors
cmake_minimum_required(VERSION 3.29)

get_filename_component(root ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
if(NOT DEST)
	set(DEST ${root}/amiga-emulation)
endif()
if(NOT HDFTOOL)
	set(HDFTOOL ${root}/build/amiga/host/tools/hdftool/hdftool)
endif()
if(NOT AMIGA_FOREVER)
	message(FATAL_ERROR "set -DAMIGA_FOREVER=DIR: the Amiga Forever medium or installation "
		"(with \"Amiga Files/Shared\")")
endif()
foreach(sub "Amiga Files/Shared" "Shared" ".")
	if(EXISTS "${AMIGA_FOREVER}/${sub}/rom/rom.key" AND IS_DIRECTORY "${AMIGA_FOREVER}/${sub}/hdf")
		get_filename_component(shared "${AMIGA_FOREVER}/${sub}" ABSOLUTE)
		break()
	endif()
endforeach()
if(NOT shared)
	message(FATAL_ERROR "${AMIGA_FOREVER}: no Amiga Forever files (\"Amiga Files/Shared/rom\" "
		"with rom.key and \"Amiga Files/Shared/hdf\")")
endif()
if(NOT EXISTS ${HDFTOOL})
	message(FATAL_ERROR "${HDFTOOL} missing: build the host side first "
		"(cmake --build --preset amiga) or give -DHDFTOOL=")
endif()
file(MAKE_DIRECTORY ${DEST})

# the inputs: path below Shared, size, SHA-256 (Amiga Forever 11)
set(inputs
	"rom/amiga-os-310-a1200.rom|524299|541f7467bce4f4b07a6eb5250d4586a4679c2da51ad30b48ba93421a901b1bb0"
	"rom/amiga-os-310-a600.rom|524299|03ec7583249c686a3f63b5b92634b12e14ce72fc27b0066a8c3cca57f869443e"
	"rom/amiga-os-130.rom|262155|caa4910942b8d558c9ae2284405af81db3752535192cc25f7939765c3ea2d5e0"
	"rom/rom.key|1426|f3b3b35593fef9a677225f559f8155e2b5d97b5bbfb2ccf24ee93738124d9a71"
	"hdf/workbench-311.hdf|6291456|cf612f71644e204ba0df6de0c64ee3bbbfafad267f2ff92cb3f22364111b02f0"
	"hdf/workbench-135.hdf|3145728|a0297f7ddc1f7abd01400ff24b81c5c19d1a1b414f9d0bf490d8ad7d0730eb9e")
foreach(i IN LISTS inputs)
	string(REPLACE "|" ";" i "${i}")
	list(GET i 0 path)
	list(GET i 1 size)
	list(GET i 2 sha)
	set(f "${shared}/${path}")
	if(NOT EXISTS "${f}")
		message(FATAL_ERROR "${f} missing")
	endif()
	file(SIZE "${f}" n)
	file(SHA256 "${f}" s)
	if(NOT n EQUAL size OR NOT s STREQUAL sha)
		message(FATAL_ERROR "${f}: unknown file (${n} bytes, SHA-256 ${s}; expected ${size} "
			"bytes, ${sha}) - not the Amiga Forever 11 edition this script knows")
	endif()
endforeach()

# the results: name, SHA-256
set(results
	"amiga-os-310-a1200.rom|541f7467bce4f4b07a6eb5250d4586a4679c2da51ad30b48ba93421a901b1bb0"
	"amiga-os-310-a600.rom|03ec7583249c686a3f63b5b92634b12e14ce72fc27b0066a8c3cca57f869443e"
	"amiga-os-130.rom|caa4910942b8d558c9ae2284405af81db3752535192cc25f7939765c3ea2d5e0"
	"rom.key|f3b3b35593fef9a677225f559f8155e2b5d97b5bbfb2ccf24ee93738124d9a71"
	"workbench-311.hdf|e150f30f91d8b2861f9478deed90c18b9bcad1c7be0419b440dcd3a80ab1579f"
	"workbench-311-rtg.hdf|33dba185784a838450334f502539db758081c5d3aad6a161f0cbe53b122108df"
	"workbench-135.hdf|a7b4fd7e1509a6086baf04f14c44894af41107c3ee9093e585261154665d5c28")

# S:User-Startup: TEST:run.script if TEST: is mounted (without asking for
# the volume), AmigaDOS 2.0+ and 1.3 syntax
set(user_startup_31 [=[
Assign >NIL: TEST: EXISTS
If NOT WARN
  If EXISTS TEST:run.script
    Execute TEST:run.script
  EndIf
EndIf
]=])
set(user_startup_13 [=[
FAILAT 21
ASSIGN >NIL: TEST: EXISTS
IF NOT WARN
IF EXISTS TEST:run.script
EXECUTE TEST:run.script
ENDIF
ENDIF
]=])
string(REGEX REPLACE "^\n" "" user_startup_31 "${user_startup_31}")
string(REGEX REPLACE "^\n" "" user_startup_13 "${user_startup_13}")
# Workbench 1.3's Startup-Sequence knows no User-Startup: before LoadWB
# (no ';' in these texts: they pass through a CMake list)
set(ss13_old "\n\nLoadWB DELAY")
set(ss13_new "\n\nIF EXISTS S:User-Startup\nExecute S:User-Startup\nENDIF\n\nLoadWB DELAY")

# make DEST/name as NAME.part with the command after the name (the
# placeholder @OUT@ for the output path; COPY: copy the input), check it
function(make_result name)
	foreach(r IN LISTS results)
		string(REPLACE "|" ";" r "${r}")
		list(GET r 0 n)
		if(n STREQUAL name)
			list(GET r 1 sha)
		endif()
	endforeach()
	set(out ${DEST}/${name})
	if(EXISTS ${out})
		file(SHA256 ${out} s)
		if(s STREQUAL sha)
			message(STATUS "${name}: up to date")
			return()
		endif()
		if(NOT FORCE)
			message(FATAL_ERROR "${out} exists with other contents (SHA-256 ${s}); not "
				"overwritten - move it away or give -DFORCE=ON")
		endif()
	endif()
	set(part ${out}.part)
	file(REMOVE ${part})
	if(ARGV1 STREQUAL "COPY")
		file(COPY_FILE "${ARGV2}" ${part})
	else()
		set(cmd ${ARGN})
		list(TRANSFORM cmd REPLACE "^@OUT@$" "${part}")
		execute_process(COMMAND ${HDFTOOL} ${cmd} RESULT_VARIABLE rc)
		if(NOT rc EQUAL 0)
			file(REMOVE ${part})
			message(FATAL_ERROR "${name}: hdftool failed (${rc})")
		endif()
	endif()
	file(SHA256 ${part} s)
	if(NOT s STREQUAL sha)
		file(REMOVE ${part})
		message(FATAL_ERROR "${name}: the result has SHA-256 ${s}, expected ${sha}")
	endif()
	file(CHMOD ${part} PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
	file(RENAME ${part} ${out})
	message(STATUS "${name}: written")
endfunction()

make_result(amiga-os-310-a1200.rom COPY "${shared}/rom/amiga-os-310-a1200.rom")
make_result(amiga-os-310-a600.rom COPY "${shared}/rom/amiga-os-310-a600.rom")
make_result(amiga-os-130.rom COPY "${shared}/rom/amiga-os-130.rom")
make_result(rom.key COPY "${shared}/rom/rom.key")
make_result(workbench-311.hdf edit "${shared}/hdf/workbench-311.hdf" @OUT@ --partition
	--write S/User-Startup "${user_startup_31}")
make_result(workbench-311-rtg.hdf edit "${shared}/hdf/workbench-311.hdf" @OUT@ --partition
	--write S/User-Startup "${user_startup_31}"
	--copy Storage/Monitors/UAE Devs/Monitors/UAE
	--copy Storage/Monitors/UAE.info Devs/Monitors/UAE.info)
make_result(workbench-135.hdf edit "${shared}/hdf/workbench-135.hdf" @OUT@
	--replace S/Startup-Sequence "${ss13_old}" "${ss13_new}"
	--write S/User-Startup "${user_startup_13}")

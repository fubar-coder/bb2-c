# The staging directory of the reference build (refbuild.cmake), made
# anew; cdrun mounts it as the "CD" (docs/rwelibs/README.md 3.1):
#   BLITZLIBS/     (blitzlibs:, libincs:, blitzincs:) the files of the
#                  CD's EXTRAS/BLITZLIBS/ (libmacs, residents), its
#                  LIBNUMS.BB replaced by LIBNUMS (with the RWE numbers),
#                  sbra.res = SBRA (BUM10A/BLITZLIBS/ASMMACS.RES, Steve
#                  Matty's short branch macros), the RESIDENTS
#   LIBMACS.BB     for XINCLUDE "libmacs.bb": cdrun runs a job in the
#                  nearest directory that has a relative include
#   optidec.bin    INCBIN of stclib (and stcdecrunchlib), whose jobs run
#                  in the root for libmacs.bb
#   FNS/           (FNS:) PERSONAL.FNS, INCBIN of fns-library11
#   BLITZ2/        (added to blitz2:) Debugger/Font.bin = LESFONT
#                  (rilesdebuglib-font.bin), INCBIN
#                  "Blitz2:Debugger/Font.bin" of LESDEBUGlib
#   RWE/REL        the SOURCES (REL|FILE|XTRA: FILE staged as RWE/REL,
#                  XTRA - the .xtra next to the source - as RWE/REL.xtra
#                  if not empty)
#   cmake -DSTAGE=... -DBLITZLIBS=CD/EXTRAS/BLITZLIBS -DLIBNUMS=... -DSBRA=...
#         -DRESIDENTS=... -DOPTIDEC=... -DFNSFONT=... -DLESFONT=...
#         -DSOURCES=... -P refbuild-stage.cmake
foreach(v STAGE BLITZLIBS LIBNUMS SBRA RESIDENTS OPTIDEC FNSFONT LESFONT SOURCES)
	if("${${v}}" STREQUAL "")
		message(FATAL_ERROR "refbuild-stage.cmake: -D${v}=... missing")
	endif()
endforeach()
file(REMOVE_RECURSE ${STAGE})
file(MAKE_DIRECTORY ${STAGE}/BLITZLIBS ${STAGE}/FNS ${STAGE}/RWE)

# copy FROM to TO, writable (the originals are read-only)
function(stage from to)
	if(NOT EXISTS ${from})
		message(FATAL_ERROR "refbuild-stage.cmake: no ${from}")
	endif()
	get_filename_component(dir ${to} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	file(COPY_FILE ${from} ${to})
	file(CHMOD ${to} FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
endfunction()

file(GLOB cdfiles LIST_DIRECTORIES false ${BLITZLIBS}/*)
foreach(f IN LISTS cdfiles)
	get_filename_component(n ${f} NAME)
	string(TOUPPER "${n}" upper)
	if(NOT upper STREQUAL "LIBNUMS.BB")
		stage(${f} ${STAGE}/BLITZLIBS/${n})
	endif()
	if(upper STREQUAL "LIBMACS.BB")
		stage(${f} ${STAGE}/${n})
	endif()
endforeach()
stage(${LIBNUMS} ${STAGE}/BLITZLIBS/LIBNUMS.BB)
stage(${SBRA} ${STAGE}/BLITZLIBS/sbra.res)
foreach(r IN LISTS RESIDENTS)
	get_filename_component(n ${r} NAME)
	stage(${r} ${STAGE}/BLITZLIBS/${n})
endforeach()
stage(${OPTIDEC} ${STAGE}/optidec.bin)
get_filename_component(n ${FNSFONT} NAME)
stage(${FNSFONT} ${STAGE}/FNS/${n})
stage(${LESFONT} ${STAGE}/BLITZ2/Debugger/Font.bin)
foreach(s IN LISTS SOURCES)
	string(REPLACE "|" ";" s "${s}")
	list(GET s 0 rel)
	list(GET s 1 file)
	stage(${file} ${STAGE}/RWE/${rel})
	list(LENGTH s n)
	if(n GREATER 2)
		list(GET s 2 xtra)
		stage(${xtra} ${STAGE}/RWE/${rel}.xtra)
	endif()
endforeach()

# The sources of bblibsrc.lha (Aminet amibl2src.lha), unpacked by
# cmake/fetch-originals.cmake.  Not sources and left out: NAME.xtra (TED's
# editor state of NAME), *.bin and *.inc (data for IncBin: arc.bin,
# font.bin, sqlup2.inc), *.obj (debuglib.obj, an object file).
#   bblibsrc_files(REPO DIRVAR FILESVAR [CONFIGURE_DEPENDS])
#   FILESVAR: the names, sorted; empty without the archive
function(bblibsrc_files repo dirvar filesvar)
	set(dir ${repo}/archive/aminet/amibl2src/bblibsrc)
	file(GLOB files RELATIVE ${dir} ${ARGN} ${dir}/*)
	list(FILTER files EXCLUDE REGEX "\\.(xtra|bin|inc|obj)$")
	list(SORT files)
	set(${dirvar} ${dir} PARENT_SCOPE)
	set(${filesvar} ${files} PARENT_SCOPE)
endfunction()

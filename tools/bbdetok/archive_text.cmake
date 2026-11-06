# Build target archive_text (included by the top-level CMakeLists.txt): the
# tokenised sources of archive/sources/ (AmiBlitz 2.41 ... AmiBlitz3
# 3.00.19a) as plain text, for reading and diffing the snapshots,
#   build/<preset>/archive-text/PATH.txt    line N = line N of the source
# for archive/sources/PATH (*.bb2).  The tokens are AmiBlitz 2.42's - its
# compiler, AcidLibs and Deflibs from the Aminet distribution
# (archive/aminet/ab2.42-distri, cmake -P cmake/fetch-originals.cmake),
# the release closest to these sources that the archive has; bbdetok
# --strict fails on a token they do not have, the file is then not
# written and the step fails.  Part of ALL when the distribution is there
# (the glob notices new sources at the next build), else the target only
# says so.
set(ARCHIVE_TEXT_DIR ${CMAKE_BINARY_DIR}/archive-text)
set(ab242 ${PROJECT_SOURCE_DIR}/archive/aminet/ab2.42-distri/Amiblitz2)
set(ab242_files ${ab242}/Amiblitz2 ${ab242}/AcidLibs ${ab242}/Deflibs)
set(sources_dir ${PROJECT_SOURCE_DIR}/archive/sources)
file(GLOB_RECURSE archive_text_in RELATIVE ${sources_dir} CONFIGURE_DEPENDS ${sources_dir}/*.bb2)
# plain text already (LF, no 0 byte): the compiler GUI's settings
list(REMOVE_ITEM archive_text_in amiblitz2-2.50beta/Blitz2/Developer/amiblitz2/compiler_gui.bb2)
list(SORT archive_text_in)
set(have_ab242 TRUE)
foreach(f IN LISTS ab242_files)
	if(NOT EXISTS ${f})
		set(have_ab242 FALSE)
	endif()
endforeach()
if(have_ab242 AND archive_text_in)
	set(outs)
	foreach(name IN LISTS archive_text_in)
		set(out ${ARCHIVE_TEXT_DIR}/${name}.txt)
		get_filename_component(outdir ${out} DIRECTORY)
		file(MAKE_DIRECTORY ${outdir})
		add_custom_command(OUTPUT ${out}
			COMMAND ${BBDETOK} --strict --blitz2 ${ab242}/Amiblitz2 --lib ${ab242}/AcidLibs
				--lib ${ab242}/Deflibs ${sources_dir}/${name} ${out}
			DEPENDS ${sources_dir}/${name} ${ab242_files} ${BBDETOK} host_tools
			COMMENT "Detokenising archive/sources/${name}"
			VERBATIM)
		list(APPEND outs ${out})
	endforeach()
	add_custom_target(archive_text ALL DEPENDS ${outs})
else()
	add_custom_target(archive_text
		COMMAND ${CMAKE_COMMAND} -E echo "archive_text: no ${ab242} (cmake -P cmake/fetch-originals.cmake): skipped"
		VERBATIM)
endif()

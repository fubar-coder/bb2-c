# TED-C's screen fonts of 256 glyphs (spec tooling/ted-font): the glyphs
# 0-127 of TED 1.1 (archive/sources/ted-1.1/chars8.b, chars.b) and the
# glyph text sources here (glyphs8.txt, glyphs6.txt: $80-$FF, the 6 px
# glyph 26), joined by the host tool tedfont (tools/tedfont) into DIR:
#   ted_font8.b  2048 bytes, 8 x 8
#   ted_font6.b  1536 bytes, 8 x 6
# The check is the second command of the same build step: a font that
# fails it is removed, so nothing that depends on it is built.
#
#   ted_fonts(TARGET TOOL DIR [DEPENDS...])
#     TARGET   a custom target of the two fonts (add_dependencies on it)
#     TOOL     the tedfont program
#     DIR      the output directory
#     DEPENDS  more dependencies of the step (the target that builds TOOL)
# The font files are set in the parent scope as TED_FONT8 and TED_FONT6,
# DIR as TED_FONTS_DIR.  ted_fonts_broken_test(NAME TOOL DIR) adds the test
# NAME: the step on a source broken on purpose fails, and no font is left.
set(TED_FONTS_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(TED_FONTS_ARCHIVE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../archive/sources/ted-1.1
	ABSOLUTE)

# the commands of the step for height H: source SRC, output OUT
function(ted_fonts_commands var tool h src out)
	if(h EQUAL 8)
		set(archive ${TED_FONTS_ARCHIVE_DIR}/chars8.b)
	else()
		set(archive ${TED_FONTS_ARCHIVE_DIR}/chars.b)
	endif()
	set(${var}
		COMMAND ${tool} build ${h} ${archive} ${src} ${out}
		COMMAND ${tool} check ${h} ${archive} ${src} ${out}
		PARENT_SCOPE)
	set(${var}_archive ${archive} PARENT_SCOPE)
endfunction()

function(ted_fonts target tool dir)
	set(outs)
	foreach(h 8 6)
		set(src ${TED_FONTS_SRC_DIR}/glyphs${h}.txt)
		set(out ${dir}/ted_font${h}.b)
		ted_fonts_commands(cmds ${tool} ${h} ${src} ${out})
		add_custom_command(OUTPUT ${out}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${dir}
			${cmds}
			DEPENDS ${tool} ${src} ${cmds_archive} ${ARGN}
			COMMENT "TED-C's ${h} px font (tedfont)"
			VERBATIM)
		list(APPEND outs ${out})
	endforeach()
	add_custom_target(${target} DEPENDS ${outs})
	set(TED_FONT8 ${dir}/ted_font8.b PARENT_SCOPE)
	set(TED_FONT6 ${dir}/ted_font6.b PARENT_SCOPE)
	set(TED_FONTS_DIR ${dir} PARENT_SCOPE)
endfunction()

function(ted_fonts_broken_test name tool dir)
	add_test(NAME ${name} COMMAND ${CMAKE_COMMAND} -DTOOL=${tool} -DDIR=${dir}
		-DSRC_DIR=${TED_FONTS_SRC_DIR} -DARCHIVE_DIR=${TED_FONTS_ARCHIVE_DIR}
		-P ${TED_FONTS_SRC_DIR}/fonts-broken.cmake)
	set_tests_properties(${name} PROPERTIES
		PASS_REGULAR_EXPRESSION "the build step refused every broken input")
endfunction()

# blitz2:deflibs - the AmigaOS call declarations (one C file per AmigaOS
# library, generated once from the CD's deflibs).  Every NAME.c is built like
# a library (target deflib_NAME, file deflib_NAME.obj - prefixed because e.g.
# mathtranslib is also a command library) and all of them are joined in the
# order of the original file (= AmigaDOS hash order of the directory
# makedeflibs.bb2 read).
set(DEFLIBS_ORDER
	ciablib disklib amigaguidelib asllib iffparselib colorwheellib doslib
	mathieeedoubbaslib mathieeedoubtranslib doslib2 mathtranslib keymaplib
	mathieeesingbaslib iconlib crmlib expansionlib localelib commoditieslib
	execlib mathffplib ciaalib datatypeslib rexxsyslib batmemlib xprzmodemlib
	intuitionlib misclib graphicslib bulletlib graphicslib2 layerslib wblib
	cardreslib utilitylib battclocklib gadtoolslib diskfontlib
	mathieeesingtranslib potgolib)
file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
foreach(src ${sources})
	get_filename_component(name ${src} NAME_WE)
	if(NOT name IN_LIST DEFLIBS_ORDER)
		message(FATAL_ERROR "libs/deflibs/${name}.c is not in DEFLIBS_ORDER")
	endif()
endforeach()

# the libraries and deflibs of the default build (prefix "", variant "")
# or of a variant; the join checks that no number of deflibs is also in
# the acidlibs ACIDLIBS loaded with it
function(deflibs_build prefix variant output acidlibs)
	set(objs)
	set(varg)
	if(variant)
		set(varg VARIANT ${variant})
	endif()
	foreach(name ${DEFLIBS_ORDER})
		blitz_library(${prefix}deflib_${name} ${varg} ${name}.c)
		list(APPEND objs ${CMAKE_CURRENT_BINARY_DIR}/${prefix}deflib_${name}.obj)
	endforeach()
	biglib_join(${prefix}deflibs ${output} ${objs} CHECK ${acidlibs})
endfunction()

deflibs_build("" "" ${BLITZ2_DIR}/deflibs ${BLITZ2_DIR}/acidlibs)
foreach(v IN LISTS BLITZ_VARIANTS)
	deflibs_build(${v}- ${PROJECT_SOURCE_DIR}/libs/variants/${v} ${BLITZ_VARIANTS_DIR}/${v}/deflibs
		${BLITZ_VARIANTS_DIR}/${v}/acidlibs)
endforeach()

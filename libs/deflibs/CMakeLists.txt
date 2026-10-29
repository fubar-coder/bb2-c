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

# the libraries and deflibs of a variant (VARIANT: its directory; PREFIX ""
# for the default variant); the join checks that no number of deflibs is
# also in the acidlibs ACIDLIBS loaded with it
function(deflibs_build prefix variant output acidlibs)
	set(objs)
	foreach(name ${DEFLIBS_ORDER})
		set(obj ${CMAKE_CURRENT_BINARY_DIR}/deflib_${name}.obj)
		if(prefix)
			set(obj ${CMAKE_CURRENT_BINARY_DIR}/variants/${prefix}deflib_${name}.obj)
		endif()
		blitz_library(${prefix}deflib_${name} VARIANT ${variant} OBJ ${obj} ${name}.c)
		list(APPEND objs ${obj})
	endforeach()
	biglib_join(${prefix}deflibs ${output} ${objs} CHECK ${acidlibs})
endfunction()

deflibs_build("" ${BLITZ_DEFAULT_VARIANT_DIR} ${BLITZ2_DIR}/deflibs ${BLITZ2_DIR}/acidlibs)
foreach(build IN LISTS BLITZ_VARIANT_BUILDS)
	string(REPLACE "|" ";" build "${build}")
	list(GET build 0 dir)
	list(GET build 1 out)
	get_filename_component(v ${dir} NAME)
	deflibs_build(${v}- ${dir} ${out}/deflibs ${out}/acidlibs)
endforeach()

# A C header with the offsets of labels of the reference TED (ref/ted.labels:
# "ADDR<tab>NAME<tab>FILE:LINE", ADDR = offset in the code hunk).
#   cmake -DLABELS=ted.labels -DOUT=ted_labels.h -DNAMES="a;b" -P labels.cmake
# Every name must be there exactly once.
cmake_minimum_required(VERSION 3.28)

file(STRINGS ${LABELS} lines)
set(defs "")
foreach(name IN LISTS NAMES)
	set(found "")
	foreach(l IN LISTS lines)
		if(l MATCHES "^([0-9A-F]+)\t${name}\t")
			list(APPEND found ${CMAKE_MATCH_1})
		endif()
	endforeach()
	list(LENGTH found n)
	if(NOT n EQUAL 1)
		message(FATAL_ERROR "${LABELS}: label ${name} found ${n} times")
	endif()
	string(APPEND defs "#define TED_${name} 0x${found}UL\n")
endforeach()

get_filename_component(from ${LABELS} NAME)
file(WRITE ${OUT} "/* generated from ${from} by labels.cmake */
#ifndef TED_LABELS_H
#define TED_LABELS_H
${defs}#endif
")

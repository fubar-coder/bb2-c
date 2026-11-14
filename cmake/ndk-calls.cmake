# The NDK functions with a parameter in a3, a4 or a5 and the libraries'
# shadow headers (docs/libs/cmdlibs-port.md, "OS calls with a parameter in
# a3, a4 or a5").  The libraries are compiled with -ffixed-a3/a4/a5; an
# NDK inline call that loads one of these registers returns with it
# changed (gcc saves no fixed register around an __asm block).
#
#   ndk_scan(INLINE_DIR OUT_FUNCS OUT_VA OUT_BASES)
#     reads the NDK's inline/*.h (not the vbcc *_protos.h).  OUT_FUNCS:
#     "HEADER NAME REGS" for every macro that loads a3/a4/a5 (register
#     variable __asm("aN")) or exchanges one around its jsr (exg %/dN,%/aN,
#     the NDK's own protection), REGS like a3,a4; OUT_VA: "HEADER NAME VA"
#     for every varargs wrapper __VA_va that calls __NAME_base; OUT_BASES:
#     "HEADER NAME" for every NAME the header defines as __NAME_base
#   ndk_read_list(LIST OUT)
#     the project's list (libs/include/ndk/functions.list): "HEADER NAME
#     REGS STATE" per function; a line of another form is a FATAL_ERROR
#   ndk_check_list(INLINE_DIR LIST)
#     compares the scan with the list in both directions (name, header,
#     registers; native only for a5, a5 only native, every function safe,
#     poisoned or native): a difference is a
#     FATAL_ERROR naming the function and the header.  Run at configure
#     (cmake/BlitzLibrary.cmake), so that an NDK of a toolchain update
#     cannot bring a function with a parameter in a3/a4 the list lacks
#   cmake -DINLINE=DIR -DLIST=FILE [-DDROP=NAME] [-DADD=ENTRY] -P ...
#     the check alone, with NAME taken out of the list or ENTRY added in
#     memory (the ctests ndk_list_*)
#   ndk_shadow_headers(INLINE_DIR LIST SAFE_DIR OUT_DIR)
#     writes OUT_DIR/inline/X.h for every NDK inline/X.h (openspec change
#     ndk-safe-calls, design D1-D3): it includes the NDK's header (its
#     varargs wrappers static inline), then
#     redefines __NAME_base of every safe NAME by SAFE_DIR/X.h and poisons
#     every poisoned NAME, with the varargs forms of both.  A file is
#     written only when its text changes (no recompilation otherwise)
#
# The states of the list:
#   safe      SAFE_DIR/HEADER.h redefines __NAME_base with the register
#             saved around the call (exg, as the NDK does for a5)
#   poisoned  a call does not compile (#pragma GCC poison)
#   native    the NDK saves the register itself (a5: exg %/d7,%/a5)
# Any other state (such as "pending": neither safe nor poisoned) is
# refused, by ndk_read_list and by ndk_check.
set(NDK_CALLS_STATES safe poisoned native)

function(ndk_scan dir out_funcs out_va out_bases)
	file(GLOB headers ${dir}/*.h)
	set(funcs)
	set(va)
	set(bases)
	foreach(h IN LISTS headers)
		get_filename_component(hname ${h} NAME_WE)
		if(hname MATCHES "_protos$")
			continue()
		endif()
		# one list element per line, without the characters that mean
		# something in a CMake list (the continuation backslashes, ; [ ])
		file(READ ${h} text)
		string(REGEX REPLACE "[][;\\]" "" text "${text}")
		string(REPLACE "\n" ";" lines "${text}")
		list(FILTER lines INCLUDE REGEX
			"^#define [A-Za-z0-9_]+\\(|__asm\\(\"a[345]\"\\)|exg %/d[0-7],%/a[345]|^static __stdargs .*__[A-Za-z0-9_]+_va\\(|^ *(return )?__[A-Za-z0-9_]+_base\\(")
		set(name)
		set(wrapper)
		set(names)
		foreach(l IN LISTS lines)
			if(l MATCHES "^#define ([A-Za-z0-9_]+)\\(")
				set(name ${CMAKE_MATCH_1})
				if(name MATCHES "^__(.+)_base$")
					set(name ${CMAKE_MATCH_1})
					list(APPEND bases "${hname} ${name}")
				endif()
			else()
				# one MATCHES per if(): an OR evaluates both and the
				# second clears CMAKE_MATCH_1
				set(r)
				if(l MATCHES "__asm\\(\"(a[345])\"\\)")
					set(r ${CMAKE_MATCH_1})
				elseif(l MATCHES "exg %/d[0-7],%/(a[345])")
					set(r ${CMAKE_MATCH_1})
				endif()
				if(r AND name AND NOT l MATCHES "^static ")
					list(APPEND regs_${name} ${r})
					list(APPEND names ${name})
				endif()
			endif()
			if(l MATCHES "^static __stdargs .*__([A-Za-z0-9_]+)_va\\(")
				set(wrapper ${CMAKE_MATCH_1})
			elseif(wrapper AND l MATCHES "^ *(return )?__([A-Za-z0-9_]+)_base\\(")
				list(APPEND va "${hname} ${CMAKE_MATCH_2} ${wrapper}")
				set(wrapper)
			endif()
		endforeach()
		list(REMOVE_DUPLICATES names)
		foreach(n IN LISTS names)
			list(REMOVE_DUPLICATES regs_${n})
			list(SORT regs_${n})
			list(JOIN regs_${n} "," r)
			list(APPEND funcs "${hname} ${n} ${r}")
			unset(regs_${n})
		endforeach()
	endforeach()
	list(REMOVE_DUPLICATES va)
	list(REMOVE_DUPLICATES bases)
	set(${out_funcs} ${funcs} PARENT_SCOPE)
	set(${out_va} ${va} PARENT_SCOPE)
	set(${out_bases} ${bases} PARENT_SCOPE)
endfunction()

function(ndk_read_list file out)
	file(STRINGS ${file} lines)
	set(entries)
	set(n 0)
	foreach(l IN LISTS lines)
		math(EXPR n "${n} + 1")
		if(l MATCHES "^[ \t]*(#|$)")
			continue()
		endif()
		string(REGEX REPLACE "[ \t]+" " " l "${l}")
		string(STRIP "${l}" l)
		if(NOT l MATCHES "^([a-z0-9_]+) ([A-Za-z0-9_]+) (a[345](,a[345])*) ([a-z]+)$")
			message(FATAL_ERROR "${file}:${n}: not HEADER NAME REGS STATE: ${l}")
		endif()
		set(state ${CMAKE_MATCH_5})
		if(NOT state IN_LIST NDK_CALLS_STATES)
			message(FATAL_ERROR "${file}:${n}: ${CMAKE_MATCH_2}: unknown state ${state} (${NDK_CALLS_STATES})")
		endif()
		list(APPEND entries "${l}")
	endforeach()
	set(${out} ${entries} PARENT_SCOPE)
endfunction()

# write FILE with TEXT unless it holds it already
function(ndk_write_if_changed file text)
	if(EXISTS ${file})
		file(READ ${file} old)
		if(old STREQUAL text)
			return()
		endif()
	endif()
	file(WRITE ${file} "${text}")
endfunction()

function(ndk_shadow_headers dir list safe_dir out_dir)
	ndk_scan(${dir} funcs va bases)
	ndk_read_list(${list} entries)
	file(GLOB headers ${dir}/*.h)
	file(MAKE_DIRECTORY ${out_dir}/inline)
	foreach(h IN LISTS headers)
		get_filename_component(hname ${h} NAME_WE)
		if(hname MATCHES "_protos$")
			continue()
		endif()
		set(safe)
		set(poison)
		foreach(e IN LISTS entries)
			if(NOT e MATCHES "^${hname} ([A-Za-z0-9_]+) [^ ]+ (safe|poisoned)$")
				continue()
			endif()
			set(f ${CMAKE_MATCH_1})
			set(state ${CMAKE_MATCH_2})
			# the varargs forms reach the NDK's own __f_base, which they
			# expanded at their definition: poisoned also for a safe f
			foreach(v IN LISTS va)
				if(v MATCHES "^${hname} ${f} ([A-Za-z0-9_]+)$")
					list(APPEND poison ${CMAKE_MATCH_1} __${CMAKE_MATCH_1}_va)
				endif()
			endforeach()
			if(state STREQUAL "safe")
				if(NOT "${hname} ${f}" IN_LIST bases)
					message(FATAL_ERROR "${list}: ${f} (${hname}.h) is safe, but the NDK has no __${f}_base to redefine")
				endif()
				list(APPEND safe ${f})
			else()
				list(APPEND poison ${f})
				if("${hname} ${f}" IN_LIST bases)
					list(APPEND poison __${f}_base)
				endif()
			endif()
		endforeach()
		string(TOUPPER "${hname}" guard)
		set(text "/* Generated by cmake/ndk-calls.cmake from the NDK's <inline/${hname}.h>
   and libs/include/ndk/functions.list: do not edit.  The libraries' view
   of the NDK header (docs/libs/cmdlibs-port.md, \"OS calls with a
   parameter in a3, a4 or a5\"). */
#ifndef BLITZLIB_NDK_INLINE_${guard}_H
#define BLITZLIB_NDK_INLINE_${guard}_H

/* the varargs wrappers (static __stdargs, noinline: -fno-toplevel-reorder
   would keep every unused one) become static inline, so that only the
   ones a library calls are emitted, with the same code.  The NDK's
   inline headers use __stdargs for nothing else; the clib prototypes
   that use it come in through proto/X.h before (a prototype read here
   would not compile, nothing changes silently) */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \"-Wattributes\"
#pragma push_macro(\"__stdargs\")
#undef __stdargs
#define __stdargs inline __attribute__((__stkparm__))
#include_next <inline/${hname}.h>
#pragma pop_macro(\"__stdargs\")
#pragma GCC diagnostic pop
")
		if(safe)
			set(sfile ${safe_dir}/${hname}.h)
			if(NOT EXISTS ${sfile})
				message(FATAL_ERROR "${list}: ${safe} (${hname}.h) safe, but there is no ${sfile}")
			endif()
			file(READ ${sfile} stext)
			string(APPEND text "\n/* the safe forms: a3/a4 saved around the call (libs/include/ndk/safe/${hname}.h) */\n")
			foreach(f IN LISTS safe)
				if(NOT stext MATCHES "\n#define __${f}_base\\(")
					message(FATAL_ERROR "${sfile}: no #define __${f}_base( for the safe ${f}")
				endif()
				string(APPEND text "#undef __${f}_base\n")
			endforeach()
			string(APPEND text "${stext}")
		endif()
		if(poison)
			string(APPEND text "
/* a3/a4 parameter without a safe form: a library may not call these (the
   NDK's macro would return with a3/a4 changed).  A library that needs one
   gets its safe form in libs/include/ndk/safe/${hname}.h, state safe in
   libs/include/ndk/functions.list */
")
			foreach(p IN LISTS poison)
				string(APPEND text "#undef ${p}\n")
			endforeach()
			list(JOIN poison " " p)
			string(APPEND text "#pragma GCC poison ${p}\n")
		endif()
		string(APPEND text "#endif\n")
		ndk_write_if_changed(${out_dir}/inline/${hname}.h "${text}")
	endforeach()
endfunction()

# compare the NDK's functions with the list's ENTRIES ("HEADER NAME REGS
# STATE"); every difference is named, then a FATAL_ERROR
function(ndk_check dir entries listname)
	ndk_scan(${dir} funcs va bases)
	set(errors)
	set(listed)
	foreach(e IN LISTS entries)
		string(REGEX MATCH "^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$" m "${e}")
		set(h ${CMAKE_MATCH_1})
		set(f ${CMAKE_MATCH_2})
		set(r ${CMAKE_MATCH_3})
		set(state ${CMAKE_MATCH_4})
		if("${h} ${f} ${r}" IN_LIST listed)
			list(APPEND errors "${f} (inline/${h}.h) is in ${listname} twice")
		endif()
		list(APPEND listed "${h} ${f} ${r}")
		if(NOT state IN_LIST NDK_CALLS_STATES)
			list(APPEND errors "${f} (inline/${h}.h) has the state ${state}, not one of ${NDK_CALLS_STATES}")
		endif()
		if(NOT "${h} ${f} ${r}" IN_LIST funcs)
			list(APPEND errors "${f} (inline/${h}.h, ${r}) is in ${listname}, but the NDK declares no such function with a parameter in ${r}")
		endif()
		if(r STREQUAL "a5")
			if(NOT state STREQUAL "native")
				list(APPEND errors "${f} (inline/${h}.h): an a5 function the NDK protects is native, not ${state}")
			endif()
		elseif(state STREQUAL "native")
			list(APPEND errors "${f} (inline/${h}.h): the NDK does not save ${r}, it cannot be native")
		endif()
	endforeach()
	foreach(f IN LISTS funcs)
		if(NOT f IN_LIST listed)
			string(REGEX MATCH "^([^ ]+) ([^ ]+) ([^ ]+)$" m "${f}")
			list(APPEND errors "${CMAKE_MATCH_2} (inline/${CMAKE_MATCH_1}.h) has a parameter in ${CMAKE_MATCH_3}, but ${listname} lacks it")
		endif()
	endforeach()
	if(errors)
		list(JOIN errors "\n  " text)
		message(FATAL_ERROR "the NDK's functions with a parameter in a3, a4 or a5 "
			"(${dir}) and ${listname} differ:\n  ${text}\n"
			"(docs/libs/cmdlibs-port.md, \"OS calls with a parameter in a3, a4 or a5\")")
	endif()
endfunction()

function(ndk_check_list dir list)
	ndk_read_list(${list} entries)
	ndk_check(${dir} "${entries}" ${list})
endfunction()

# cmake -DINLINE=DIR -DLIST=FILE [-DDROP=NAME] [-DADD="HEADER NAME REGS STATE"]
#       -P ndk-calls.cmake: the check of the list, NAME taken out of it or
#       an entry added in memory (the tests with broken input)
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
	ndk_read_list(${LIST} entries)
	if(DROP)
		list(FILTER entries EXCLUDE REGEX "^[^ ]+ ${DROP} ")
	endif()
	if(ADD)
		list(APPEND entries "${ADD}")
	endif()
	ndk_check(${INLINE} "${entries}" ${LIST})
	message(STATUS "${LIST}: the NDK's functions with a parameter in a3, a4 or a5")
endif()

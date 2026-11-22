# The warnings every target of the project's own code gets on top of its
# own -Wall line, C and C++, m68k and host: plain -Wshadow (a local over a
# file-scope variable is "shadows a global declaration", which
# -Wshadow=local does not report).  Third-party code (snitch, CLI11) is not
# a target that calls this, so it keeps its own warnings.
#
#   own_warnings(TARGET...)   the options for these targets (PRIVATE)
#   OWN_WARNINGS              the options as a list, where a target's
#                             options are built as a list
#                             (BLITZLIB_OPTIONS_AFTER,
#                             EDSTUB_COMPILE_OPTIONS) or set for a whole
#                             directory (tools/CMakeLists.txt)
#
# programs/blitz2 does not call it yet (its locals and parameters named
# like the globals `lib` and `zero` still warn); it is switched on by
# own_warnings() for its targets next to their -Wall.
#
# Included by the top-level CMakeLists.txt and by host/CMakeLists.txt.
set(OWN_WARNINGS -Wshadow)

function(own_warnings)
	foreach(t IN LISTS ARGN)
		target_compile_options(${t} PRIVATE ${OWN_WARNINGS})
	endforeach()
endfunction()

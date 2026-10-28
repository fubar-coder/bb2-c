# m68k helpers of the bug-orig tests
#   inject   runs the compiler with faults injected (pass file key wrap=)
#   bugorig_userlib.obj   a Blitz library for blitz2:userlibs (file=)
add_executable(inject inject.c)
target_compile_options(inject PRIVATE -Wall -Wno-pointer-sign)
set_target_properties(inject PROPERTIES SUFFIX "")
blitz_library(bugorig_userlib userlib.c)

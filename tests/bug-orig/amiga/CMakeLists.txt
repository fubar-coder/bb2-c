# m68k helpers of the bug-orig tests
#   inject   runs the compiler or a program with faults injected (pass file
#            keys wrap=, runwrap=), counts what it leaves, gives it input
#   wbrun    starts a program like the Workbench (wrap=, runwrap=)
#   bugorig_userlib.obj   a Blitz library for blitz2:userlibs (file=)
add_executable(inject inject.c)
target_compile_options(inject PRIVATE -Wall -Wno-pointer-sign)
set_target_properties(inject PROPERTIES SUFFIX "")
add_executable(wbrun wbrun.c)
target_compile_options(wbrun PRIVATE -Wall -Wno-pointer-sign)
set_target_properties(wbrun PROPERTIES SUFFIX "")
blitz_library(bugorig_userlib userlib.c)

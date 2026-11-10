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
#   memfill  prints what fresh memory holds (self test of inject fill=)
add_executable(memfill memfill.c)
target_compile_options(memfill PRIVATE -Wall -Wno-pointer-sign)
set_target_properties(memfill PROPERTIES SUFFIX "")
#   dsdays   prints what DateStamp gives (self test of inject datestamp=)
add_executable(dsdays dsdays.c)
target_compile_options(dsdays PRIVATE -Wall -Wno-pointer-sign)
set_target_properties(dsdays PROPERTIES SUFFIX "")
#   segwrite writes behind its own hunk (self test of inject guardseg); no C
#            startup, its entry is the first code of the hunk
add_executable(segwrite segwrite.c)
target_compile_options(segwrite PRIVATE -Wall -Wno-pointer-sign -ffreestanding -fno-builtin
	-fno-tree-loop-distribute-patterns -fno-toplevel-reorder -fomit-frame-pointer)
target_link_options(segwrite PRIVATE -nostartfiles -nostdlib)
set_target_properties(segwrite PROPERTIES SUFFIX "")
blitz_library(bugorig_userlib userlib.c)

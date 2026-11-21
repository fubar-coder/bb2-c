; compiler-loadalib-reloc-count-0 (#1175) - loadalib allocates the reloc
; table of a library file with the count of its reloc header: for a count
; of 0 that is AllocMem(0), which returns 0, and the editor's extalloc
; takes it as out of memory (under TED the jump to outmem: at the start
; before TED's main loop the crash of #1008, from the reload menu the
; abandoned compiler of #369; tedstub ends the session). The library
; (a hand-made file, the library of amiga/userlib.c with an empty
; HUNK_RELOC32) is in blitz2:userlibs/bugorig and in blitz2:bugorig
; (where the original looks, compiler-userlibs-doubleslash).
; Correct: a count of 0 means no relocations; the library is loaded and
; BugOrigUserLib returns 42.
; @fixed #1175
; @pass file=blitz2/userlibs/bugorig/bugorig.obj|@TESTDATA@/compiler-loadalib-reloc-count-0.obj
; @pass file=blitz2/bugorig/bugorig.obj|@TESTDATA@/compiler-loadalib-reloc-count-0.obj
NPrint BugOrigUserLib
End

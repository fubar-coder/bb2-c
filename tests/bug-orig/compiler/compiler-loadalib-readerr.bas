; compiler-loadalib-readerr (#3) - loadalib compares the result of the
; Read of the 12 byte reloc header unsigned: a read error (-1) counts as
; success, the reloc count comes from the old buffer (here 0 from the
; hunk header) and the compiler allocates 0 bytes for the relocs - the
; allocation fails and the editor's out-of-memory handler ends the
; session before the first compile.
; Correct: a read error ends the reading like a short file; the library
; (no relocs) is loaded and the program uses its function (42).
; The library is in blitz2:userlibs/bugorig and in blitz2:bugorig (where
; the original looks, compiler-userlibs-doubleslash); inject makes the
; third Read on it fail.
; @fixed #3
; @pass file=blitz2/userlibs/bugorig/bugorig.obj|@USERLIB@
; @pass file=blitz2/bugorig/bugorig.obj|@USERLIB@
; @pass wrap=@INJECT@|readfail=bugorig.obj/3
NPrint BugOrigUserLib
End

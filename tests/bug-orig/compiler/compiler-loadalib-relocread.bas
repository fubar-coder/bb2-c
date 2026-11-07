; compiler-loadalib-relocread (#993) - loadalib reads a library file in
; four steps: the 32 byte header, the library structure, the 12 byte reloc
; header and the reloc data; the result of the last Read is not checked.
; When it fails, the reloc table keeps what the new allocation held, and
; fixlibs relocates the library with these values: it adds the library's
; base at random offsets, and the library's own reloc (the address of
; BugOrigUserLib's code) is not done. The library is in
; blitz2:userlibs/bugorig and in blitz2:bugorig (where the original
; looks, compiler-userlibs-doubleslash); inject makes the fourth Read on
; it fail.
; Correct: a library whose reloc data cannot be read is left out, as one
; with a short header or structure (#3): BugOrigUserLib is unknown and the
; compile stops at its line.
; @fixed #993
; @pass file=blitz2/userlibs/bugorig/bugorig.obj|@USERLIB@
; @pass file=blitz2/bugorig/bugorig.obj|@USERLIB@
; @pass wrap=@INJECT@|readfail=bugorig.obj/4
NPrint BugOrigUserLib
End

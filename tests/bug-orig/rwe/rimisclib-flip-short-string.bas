; rimisclib-flip-short-string (#751) - Flip$ reverses the string in place
; with a DBRA over len/2 - 1 byte pairs: for a string of 0 or 1
; characters the count is -1, the DBRA swaps 65536 pairs around the
; string, and 128 KB of memory before and behind it (the program's
; string buffers, its code or the system's memory) are reversed - a crash
; or a hang.
; Correct: Flip$ of "" is "", of "x" is "x"; nothing else changes.
; @fixed #751
; @rwe rimisclib
; @pass timeout=30
a$="" : b$="x"
NPrint "before"
c$=Flip$(a$)
d$=Flip$(b$)
NPrint "[",c$,"][",d$,"]"
NPrint Flip$("abc")
End

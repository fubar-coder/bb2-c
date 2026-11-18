; rireqlib-colour-default-word (#1091) - ReqColour(colour) does not
; start the real colour requester on the colour: req.library 2.8's
; ColorRequester reads a LONG DefColour at offset 0 of its structure and
; the window at offset 4 (REQBASE.BB2's ExtendedColorRequester; checked
; with a structure built by a C program: DefColour 2 and 3 as a long at
; 0 come back from OK as 2 and 3, as a word at 0 as 0); the original's
; structure has a WORD DefColour at 0 and the window at offset 2
; (RIReqLib's `ecr`), so the requester reads the colour shifted into the
; high word, with the window pointer's high half below it, and a mangled
; window pointer at 4 with ReqOutput.  ReqColour(3) answered with OK
; returns 0, in the original (which ignores its argument) and in the
; port before the fix (#786 stored the argument as a word at 0).  The
; program runs against the real req.library 2.8 (Aminet
; util/libs/ReqLib28.lha, skipped without it) through inject, which puts
; it into LIBS: (libs=) and answers the requester with its OK gadget
; (click=, `*` for the title).
; Correct: the colour the requester started on, 3, comes back from OK.
; @fixed #1091
; @rwe rireqlib
; @pass file=libs/req.library|@REQLIB@
; @pass runwrap=@INJECT@|libs=CORPUS:libs click=*/__Ok__
; @pass timeout=60
NPrint "colour ",ReqColour(3)
End

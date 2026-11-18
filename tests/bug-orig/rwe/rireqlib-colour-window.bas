; rireqlib-colour-window (#1091) - ReqColour after ReqOutput gives the
; real colour requester a mangled window pointer: req.library 2.8's
; ColorRequester reads the window at offset 4 of its structure
; (REQBASE.BB2's ExtendedColorRequester: DefColour a long at 0, the
; window at 4); the original's structure has DefColour as a word at 0
; and the window at offset 2, so the requester reads the window's low
; word followed by 0 as the window, and the window's high word below
; the colour as DefColour.  The program opens a window on the
; Workbench, makes it the requesters' window (ReqOutput) and answers
; the colour requester with its OK gadget, against the real req.library
; 2.8 (Aminet util/libs/ReqLib28.lha, skipped without it) through
; inject (libs= puts it into LIBS:, click= answers, `*` for the title).
; In the original no requester window opens: ReqColour returns at once,
; 16387 ($4003).
; Correct: the requester opens for the window and returns the colour it
; started on, 2.
; @fixed #1091
; @rwe rireqlib
; @pass file=libs/req.library|@REQLIB@
; @pass runwrap=@INJECT@|libs=CORPUS:libs click=*/__Ok__
; @pass timeout=60
WbToScreen 0
Window 0,20,30,200,80,$000E,"RQWIN",1,2
DefaultOutput
ReqOutput 0
NPrint "colour ",ReqColour(2)
End

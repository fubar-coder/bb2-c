; freqlib-doslist-unlocked (#156) - for DRIVES, FileRequest$ walks the DOS
; list without locking it (no LockDosList, no Forbid): if it changes in
; the meantime, the requester reads freed memory.  The program runs
; through inject, which holds the DOS list locked for writing
; (doslocked) while it clicks DRIVES, then CANCEL (click=).  A requester
; that locks the list waits in LockDosList, so CANCEL is answered only
; after inject has unlocked it (1 s); one that does not lock the list
; reads it while it is locked and answers CANCEL at once.
; Own 320x256 screen (see t93_freq).
; Correct: CANCEL is answered after the DOS list was unlocked.  The
; original answers it while the list is locked.
; @configs clib
; @pass runwrap=@INJECT@|doslocked click=FQ/DRIVES click=FQ/CANCEL
; @pass timeout=120
Screen 0,0,0,320,256,2,0,"freqlib",1,2
Use Screen 0
MaxLen p$=160
MaxLen f$=64
p$="RAM:"
f$="x"
r$=FileRequest$("FQ",p$,f$)
NPrint "cancel ",Len(r$)
End

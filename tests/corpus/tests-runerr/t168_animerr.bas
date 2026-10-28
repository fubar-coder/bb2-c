; t168_animerr - animlib: LoadAnim of a missing file.  The original
; branches (BEQ) to err7 in the error check area: with runtime error
; checking the message "Can't Open Anim File" (without it the branch
; would land behind the library - not run).
NPrint "load"
LoadAnim 0,"RAM:t168_missing.anim"
NPrint "not reached"
End

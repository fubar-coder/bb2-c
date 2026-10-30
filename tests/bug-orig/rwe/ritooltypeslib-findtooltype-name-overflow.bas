; ritooltypeslib-findtooltype-name-overflow (#842) - for the bracket
; search FindToolType allocates 256 bytes and copies "(", the name and
; ")" and a 0 into it - len + 3 bytes, the length not checked.  A name of
; 300 characters writes 47 bytes behind the block.  The program runs
; through inject (runwrap=, inject guard: a pattern behind every
; allocation, checked when it is freed); the name is not a tool type of
; the icon (SYS:Prefs), so the bracket search runs.
; Correct: nothing is written behind an allocation.
; @rwe ritooltypeslib
; @pass runwrap=@INJECT@|guard
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  NPrint FindToolType(String$("x",300),1)
Else
  NPrint "no icon"
EndIf
End

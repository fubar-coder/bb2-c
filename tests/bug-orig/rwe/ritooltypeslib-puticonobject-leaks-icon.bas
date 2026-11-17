; ritooltypeslib-puticonobject-leaks-icon (#850) - PutIconObject writes
; the icon and then forgets it: the DiskObject that GetIconObject got
; from icon.library is never freed (FreeDiskObject), its memory stays
; allocated until the machine is reset.  The icon is the one of
; SYS:Prefs, written to RAM:; inject track lists the allocations left at
; the end.
; Correct: nothing of the icon is left - the 34 allocations of 45012
; bytes of the program's environment (as in
; ritooltypeslib-addfreelist-fail-frees-0).
; @fixed #850
; @rwe ritooltypeslib
; @pass runwrap=@INJECT@|track
If GetIconObject("SYS:Prefs")
  NPrint PutIconObject("RAM:bug-orig-850")
  NPrint FreeIconObject
Else
  NPrint "no icon"
EndIf
End

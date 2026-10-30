; ritooltypeslib-puticonobject-result-ignored (#851) - PutIconObject
; calls icon.library's PutDiskObject and returns -1 without looking at
; its result: a failed write is reported as success.  The icon is the
; one of SYS:Prefs, written into a directory that does not exist (no
; requester).
; Correct: PutIconObject returns 0 when PutDiskObject fails.
; @rwe ritooltypeslib
If GetIconObject("SYS:Prefs")
  NPrint "icon"
  NPrint PutIconObject("RAM:bug-orig-no-such-dir/test")
Else
  NPrint "no icon"
EndIf
End

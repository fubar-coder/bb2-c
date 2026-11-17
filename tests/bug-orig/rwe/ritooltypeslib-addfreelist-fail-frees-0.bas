; ritooltypeslib-addfreelist-fail-frees-0 (#840) - SetToolValue,
; NewToolType and IconDefTool hang their new string on the icon's FreeList
; with AddFreeList; if that fails, the error path frees address 0 (d0,
; AddFreeList's result) instead of the block, which is lost.  The program
; runs through inject: icon.library's AddFreeList allocates a MemList of
; 96 bytes when the one it has is full; the first one is allocated by
; GetDiskObject, the second (allocfail +1) by the 5th IconDefTool, which
; then fails.  inject track lists what is left at the end: with the bug
; also the 38 bytes of that IconDefTool's string (35 allocations instead
; of 34).
; Correct: the failing IconDefTool (the 5th) returns 0 and frees its block.
; @fixed #840
; @rwe ritooltypeslib
; @pass runwrap=@INJECT@|allocfail=96-96/1+1 track
DEFTYPE .l
If GetIconObject("SYS:Prefs")
  For i=1 To 6
    NPrint "deftool ",i,": ",IconDefTool(String$("x",37))
  Next
Else
  NPrint "no icon"
EndIf
End

; ritooltypeslib-addfreelist-fail-frees-0 (#840) - SetToolValue,
; NewToolType and IconDefTool hang their new string on the icon's FreeList
; with AddFreeList; if that fails, the error path frees address 0 (d0,
; AddFreeList's result) instead of the block, which is lost.  The program
; runs through inject: icon.library's AddFreeList allocates a MemList of
; 96 bytes when the one it has is full; the first one is allocated by
; GetDiskObject, the second (allocfail +1) by the 6th IconDefTool, which
; then fails.  inject track lists what is left at the end: with the bug
; also the 38 bytes of that IconDefTool's string (45 allocations instead
; of 44; the 44 are left the same without the failure).
; Correct: the failing IconDefTool returns 0 and frees its block.
; @rwe ritooltypeslib
; @pass runwrap=@INJECT@|allocfail=96-96/1+1 track
DEFTYPE .l
If GetIconObject("SYS:Prefs")
  For i=1 To 6
    r=IconDefTool(String$("x",37))
  Next
  NPrint "deftool 6: ",r
Else
  NPrint "no icon"
EndIf
End

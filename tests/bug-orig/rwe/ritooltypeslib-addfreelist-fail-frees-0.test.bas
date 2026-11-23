; ritooltypeslib-addfreelist-fail-frees-0 (#840) - SetToolValue,
; NewToolType and IconDefTool hang their new string on the icon's FreeList
; with AddFreeList; if that fails, the error path frees address 0 (d0,
; AddFreeList's result) instead of the block, which is lost.  The program
; runs through inject: icon.library's AddFreeList allocates a MemList of
; 96 bytes when the one it has is full; the first one is allocated by
; GetDiskObject, the second (allocfail +1) by the 5th IconDefTool, which
; then fails.  inject track lists what is left at the end: with the bug
; also the 38 bytes of that IconDefTool's string.
; Correct: the failing IconDefTool (the 5th) returns 0 and frees its block.
DEFTYPE .l

!TestCase{deftool}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  For i=1 To 6
    r=IconDefTool(String$("x",37))
    If i=5
      !CheckEq{r,0,"fifth_fails"}
    Else
      !CheckEq{r,-1,"others_succeed"}
    EndIf
  Next
!EndTestCase

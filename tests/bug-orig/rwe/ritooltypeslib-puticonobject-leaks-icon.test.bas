; ritooltypeslib-puticonobject-leaks-icon (#850) - PutIconObject writes
; the icon and then forgets it: the DiskObject that GetIconObject got
; from icon.library is never freed (FreeDiskObject), its memory stays
; allocated until the machine is reset.  The icon is the one of
; SYS:Prefs, written to CORPUS:; inject track lists the allocations left
; at the end.
; Correct: nothing of the icon is left (the allocations of the program's
; environment are the same as in ritooltypeslib-addfreelist-fail-frees-0).
DEFTYPE .l

!TestCase{put_and_free}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  p=PutIconObject("CORPUS:bug-orig-850")
  f=FreeIconObject
  !CheckEq{p,-1,"put"}
  !CheckEq{f,0,"free_after_put"}
!EndTestCase

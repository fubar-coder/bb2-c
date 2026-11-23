; ritooltypeslib-puticonobject-result-ignored (#851) - PutIconObject
; calls icon.library's PutDiskObject and returns -1 without looking at
; its result: a failed write is reported as success, and the icon is
; closed as after a write.  The icon is the one of SYS:Prefs, written
; into a directory that does not exist (no requester); then
; FreeIconObject shows whether the icon is still open.
; Correct (the user's decision of 2026-11-17): PutIconObject returns 0
; when PutDiskObject fails and the icon stays open - FreeIconObject
; frees it and returns -1.
DEFTYPE .l

!TestCase{write_fails}
  opened=GetIconObject("SYS:Prefs")
  !Require{opened<>0,"icon"}
  p=PutIconObject("CORPUS:bug-orig-no-such-dir/test")
  f=FreeIconObject
  !CheckEq{p,0,"put_fails"}
  !CheckEq{f,-1,"icon_still_open"}
!EndTestCase

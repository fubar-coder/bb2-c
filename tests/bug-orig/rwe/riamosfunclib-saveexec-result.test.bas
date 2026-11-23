; riamosfunclib-saveexec-result (#501) - SaveExec returns -1 on success
; and 0 if the array is no list.  If the file cannot be created it
; returns the file name's address (a bare RTS with d0 as it is).  An
; empty list is not seen as empty: its head points to the list's tail
; node, which is written as a hunk, and the file gets a header for one
; hunk.
; Correct: 0 for the file that cannot be made, 0 and no file for the
; empty list.
DEFTYPE .l
NEWTYPE .hunk
  hunktype.w
  hunkloc.l
  hunklen.l
  reloc.l
  reloclen.l
  flags.l
End NEWTYPE

!TestCase{saveexec_result}
  Dim List h.hunk(4)
  r=SaveExec("CORPUS:riamosfunclib-no-dir/x",h())
  !CheckEq{r,0,"no_file"}
  r=SaveExec("CORPUS:riamosfunclib-empty.exe",h())
  !CheckEq{r,0,"empty_list"}
  !CheckEq{FileSize("CORPUS:riamosfunclib-empty.exe"),0,"file_size"}
!EndTestCase

!TestBatchCleanup
  KillFile "CORPUS:riamosfunclib-empty.exe"
!EndTestBatchCleanup

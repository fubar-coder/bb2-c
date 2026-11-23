; riamosfunclib-bload-open-fails (#487) - BLoad into a bank erases the
; bank and reserves it anew with the length before it opens the file: a
; file that cannot be opened leaves a bank in use erased and reserved with
; the new length, and a free bank reserved.
; Correct: BLoad returns 0 and both banks stay as they were (32 bytes,
; free).
DEFTYPE .l

!TestCase{bload_open_fails}
  r=Reserve(11,32)
  r=BLoad("CORPUS:riamosfunclib-no-such-file",11,16)
  !CheckEq{r,0,"result"}
  !CheckEq{Length(11),32,"used_bank"}
  r=BLoad("CORPUS:riamosfunclib-no-such-file",12,16)
  !CheckEq{Length(12),0,"free_bank"}
!EndTestCase

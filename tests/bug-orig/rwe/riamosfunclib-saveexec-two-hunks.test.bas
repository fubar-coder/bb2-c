; riamosfunclib-saveexec-two-hunks (#503) - SaveExec patches the
; HUNK_HEADER with first hunk 0, last hunk 0 and one size slot (the sum of
; all code sizes) and writes one HUNK_END behind all hunks: a list with two
; code items gives a file LoadSeg cannot load (in the emulator the
; LoadSeg of it crashes: the session halts).
; The program does not call LoadSeg: it reads the header of the file back
; and checks the last hunk number (the fifth long of the HUNK_HEADER).
; Correct: the header says last hunk 1 (and the file loads).
DEFTYPE .l
NEWTYPE .hunk
  hunktype.w
  hunkloc.l
  hunklen.l
  reloc.l
  reloclen.l
  flags.l
End NEWTYPE

!TestCase{saveexec_two_hunks}
  Dim List h.hunk(4)
  Dim c.l(1)
  Dim hd.l(7)
  c(0)=0 : c(1)=$4E754E75
  If AddItem(h())
    h()\hunktype=$3E9 : h()\hunkloc=&c(0) : h()\hunklen=8
  EndIf
  If AddItem(h())
    h()\hunktype=$3E9 : h()\hunkloc=&c(0) : h()\hunklen=8
  EndIf
  r=SaveExec("CORPUS:riamosfunclib-two.exe",h())
  ok=ReadFile(0,"CORPUS:riamosfunclib-two.exe")
  !Require{ok,"file_read"}
  ReadMem 0,&hd(0),20
  CloseFile 0
  !CheckEq{hd(0),$3F3,"header"}
  !CheckEq{hd(4),1,"last_hunk"}
!EndTestCase

!TestBatchCleanup
  KillFile "CORPUS:riamosfunclib-two.exe"
!EndTestBatchCleanup

; fileiolib-reopen-fields-name (#138) - OpenFile, ReadFile and WriteFile
; first close the File#; when the open file has Fields, the loop that
; frees them leaves the argument pointer at 0, and the length of the new
; file name is then read from -4(0) (on the A1200's 24 bit bus the last
; long of the ROM) and the name is cut there.
; Here a file with a name of 38 characters is opened with OpenFile and
; given Fields, then opened again with ReadFile without CloseFile.
; Correct: the second open finds the file, the name stays whole.  The
; original cuts the name and does not find the file.
DEFTYPE .l

!TestCase{reopen}
  f$="CORPUS:fields-name-reopen-12345678.dat"
  !CheckEq{Len(f$),38,"name_length"}
  r=OpenFile(0,f$)
  !Require{r,"opened"}
  Fields 0,a.l
  r=ReadFile(0,f$)
  !Check{r<>0,"reopened"}
  CloseFile 0
  KillFile f$
!EndTestCase

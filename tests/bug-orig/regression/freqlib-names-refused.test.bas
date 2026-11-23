; freqlib-names-refused - the fix of #1042 end to end (the bug-orig test
; freqlib-file-name-unbounded stops before OK, because OK makes the
; original crash): FileRequest$ refuses a clicked file name that does not
; fit its file buffer (63 characters) or the MaxLen of Filename$, and OK
; then returns what the buffers held before.  freqlib-names-limits tests
; the other limits.
; The drawer CORPUS:fqname holds files of 70 and 35 characters; a line
; shows their first 27 characters (320 pixel screen).  inject clicks
; (click=, one window title per call):
;   F1  the 70-character name, then OK: refused, "x" stays
;   F2  MaxLen f$=30: the 35-character name, then OK: refused, "y" stays
; Without the fix F1 makes OK copy without end (a crash), and F2 writes
; behind the memory of f$.

DEFTYPE .l

!TestCase{names_refused}
  Screen 0,0,0,320,256,2,0,"freqlib",1,2
  Use Screen 0
  d$="CORPUS:fqname/"
  !Require{WriteFile(0,d$+"a123456789b123456789c123456789d123456789e123456789f123456789g123456789"),"file70_written"}
  CloseFile 0
  !Require{WriteFile(0,d$+"k123456789k123456789k123456789k1234"),"file35_written"}
  CloseFile 0
  MaxLen p$=160
  MaxLen f$=64
  p$="CORPUS:fqname"
  f$="x"
  r$=FileRequest$("F1",p$,f$)
  !CheckEq{Len(r$),15,"f1_result_length"}
  !CheckStrEq{r$,"CORPUS:fqname/x","f1_result"}
  !CheckEq{Len(f$),1,"f1_file_length"}
  !CheckStrEq{f$,"x","f1_file"}
  MaxLen f$=30
  p$="CORPUS:fqname"
  f$="y"
  r$=FileRequest$("F2",p$,f$)
  !CheckEq{Len(r$),15,"f2_result_length"}
  !CheckStrEq{r$,"CORPUS:fqname/y","f2_result"}
  !CheckEq{Len(f$),1,"f2_file_length"}
  !CheckStrEq{f$,"y","f2_file"}
!EndTestCase

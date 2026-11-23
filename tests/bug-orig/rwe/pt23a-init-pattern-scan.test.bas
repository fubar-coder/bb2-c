; pt23a-init-pattern-scan (#477) - mt_init finds the highest pattern
; number of the 128 song positions with one decrement of its counter per
; byte read, but a new maximum decrements it once more (SUBQ, the DBRA
; skipped): position 127 is read only behind a new maximum at 126, and
; then the counter 0 becomes $FFFF and the scan reads 65535 bytes more
; (the patterns and what lies behind them); the bytes are compared
; signed.  The sample starts lie behind the highest pattern.  Tested
; with ritrackerlib (pt23a-init-pattern-scan-ciatrklib: ciatrklib).  The
; module (generated, loaded into memory of 8 KB) has pattern 1 at
; position 126 and pattern 2 at position 127, and the byte $7F in its
; pattern data; the original then takes 127 as the highest pattern and
; clears the first long of its "samples" 128 KB behind the module.
; TrackInit runs mt_init; the program finds the play routine's data in
; its own code (the library is linked behind it: the period table, the
; four voices of 44 bytes and mt_SampleStarts behind it) and reads the
; start of sample 1 (mt_SampleStarts+0) relative to the module.
; Correct: the highest pattern is 2 - sample 1 starts at 1084 + 3 x 1024
; ($103C).
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{pattern_scan}
  here:
  n$="CORPUS:pt.mod"
  f=Open_(&n$,1005)
  !Require{f<>0,"module_opened"}
  m=AllocMem_(8192,$10002)
  !Require{m<>0,"memory"}
  r=Read_(f,m,8192)
  Close_ f
  TrackInit m
  TrackStop
  t=findpt{?here}
  !Require{t<>0,"voices_found"}
  !CheckEq{Peek.l(t+1328)-m,$103C,"sample_1_start"}
  FreeMem_ m,8192
!EndTestCase

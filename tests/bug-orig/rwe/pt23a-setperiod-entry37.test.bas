; pt23a-setperiod-entry37 (#481) - mt_SetPeriod looks the note's period
; up in finetune 0's row with 37 comparisons; a period below the table's
; lowest note 113 matches none and takes entry 37 of the voice's
; finetune row: the second entry of the next row, and for finetune -1
; (15) the word behind the table, voice 0's n_cmd word.  Tested with
; ritrackerlib (pt23a-setperiod-entry37-ciatrklib: ciatrklib).  The
; module (generated, loaded into chip memory) plays period 100 in row 0
; on voice 0 (sample 1, finetune 0) and voice 1 (sample 2, finetune 15).
; The program finds the play routine's data in its own code (the library
; is linked behind it: the period table, with the four voices of 44
; bytes behind it) and reads the period (+16) of voices 0 and 1 after
; row 0 was played. The rows are played by hand (TrackPlay: one tick;
; speed 6, row 1 at the 7th tick; the library's CIA interrupt setup is
; dead code, #571).
; Correct: the last entry of the row - 113 (finetune 0), 114 (-1).
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{entry_37}
  here:
  n$="CORPUS:pt.mod"
  f=Open_(&n$,1005)
  !Require{f<>0,"module_opened"}
  m=AllocMem_(4096,$10002)
  !Require{m<>0,"memory"}
  r=Read_(f,m,4096)
  Close_ f
  s=m+$43C+1024
  TrackInit m
  For i=1 To 13 : TrackPlay : Next
  TrackStop
  t=findpt{?here}
  !Require{t<>0,"voices_found"}
  v=t+1152
  !CheckEq{Peek.w(v+16),113,"voice_0_period"}
  !CheckEq{Peek.w(v+44+16),114,"voice_1_period"}
  FreeMem_ m,4096
!EndTestCase

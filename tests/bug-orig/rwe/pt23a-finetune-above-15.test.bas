; pt23a-finetune-above-15 (#476) - the finetune byte of a sample header
; is taken unmasked; every period lookup multiplies it by 72 bytes: a
; finetune of 16 reads behind the period table (in both builds the four
; voices follow it).  Tested with ritrackerlib (ciatrklib has the same
; code).  The module (generated, loaded into chip memory) plays sample
; 31 with finetune 16 at period 428 in row 0.  The program finds the
; play routine's data in its own code (the library is linked behind it:
; the period table, with the four voices of 44 bytes behind it) and
; reads the period (+16) of voice 0 after row 0 was played. The rows are
; played by hand (TrackPlay: one tick; speed 6, row 1 at the 7th tick;
; the library's CIA interrupt setup is dead code, #571).
; Correct: the finetune is masked to 0..15 (16 -> 0): period 428.
; The same test for ciatrklib: pt23a-finetune-above-15-ciatrklib.
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{finetune}
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
  !CheckEq{Peek.w(v+16),428,"period"}
  FreeMem_ m,4096
!EndTestCase

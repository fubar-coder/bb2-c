; pt23a-9xx-twice (#473) - for a note with 9xx mt_SetRegs runs
; mt_CheckMoreEfx (mt_SampleOffset: start + 2 x offset, length - offset)
; and then mt_SetPeriod, which writes start and length to Paula and ends
; in mt_CheckMoreEfx again: the voice keeps the offset twice (a retrig,
; a note delay and a following note without a sample number start at the
; doubled offset).  Tested with ritrackerlib (pt23a-9xx-twice-ciatrklib:
; ciatrklib).  The module (generated, loaded into chip memory) plays
; sample 31 of 2000 words with 902 (256 words) in row 0.  The program
; finds the play routine's data in its own code (the library is linked
; behind it: the period table, with the four voices of 44 bytes behind
; it) and reads the start (+4) and the length (+8) of voice 0 after row
; 0 was played.  The rows are played by hand (TrackPlay: one tick; speed
; 6, row 1 at the 7th tick; the library's CIA interrupt setup is dead
; code, #571).
; Correct: the offset once - start + 512 bytes, length 2000 - 256.
; @fixed #473
; @rwe ritrackerlib
; @pass file=pt.mod|@TESTDATA@/pt23a-9xx-twice.mod
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function
here:
n$="CORPUS:pt.mod"
f=Open_(&n$,1005)
If f=0 Then NPrint "no module" : End
m=AllocMem_(4096,$10002)
r=Read_(f,m,4096)
Close_ f
s=m+$43C+1024
TrackInit m
For i=1 To 13 : TrackPlay : Next
TrackStop
t=findpt{?here}
NPrint "voices found ",t<>0
v=t+1152
NPrint "start +",Peek.l(v+4)-s," length ",Peek.w(v+8)
FreeMem_ m,4096
End

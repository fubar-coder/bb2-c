; pt23a-samplenum-over-31 (#480) - the sample number of a note (up to
; 255) indexes the table of the sample starts (32 longs, 31 set) and the
; sample headers without a check: sample 33 takes its start from behind
; the table - mt_SongDataPtr, the module's own address.  Tested with
; ritrackerlib (ciatrklib has the same code).  The module (generated,
; loaded into chip memory) plays sample 1 in row 0 and "sample 33" in
; row 1.  The program finds the play routine's data in its own code (the
; library is linked behind it: the period table, with the four voices of
; 44 bytes behind it) and reads the start (+4) of voice 0 after row 1
; was played. The rows are played by hand (TrackPlay: one tick; speed 6,
; row 1 at the 7th tick; the CIA interrupt does not play in the
; emulator).
; Correct: a sample number above 31 sets no new sample - the start stays
; the one of sample 1.
; @rwe ritrackerlib
; @pass file=pt.mod|@TESTDATA@/pt23a-samplenum-over-31.mod
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
p=Peek.l(v+4)
NPrint "sample 1 ",p=s," module ",p=m
FreeMem_ m,4096
End

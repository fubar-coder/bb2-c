; pt23a-9xx-signed-length (#472) - 9xx compares the offset with the
; sample length signed (CMP.W, BGE): a sample of 32768 words or more
; counts as shorter than any offset - the length is set to 1 and the
; start is not moved.  Tested with ritrackerlib (ciatrklib has the same
; code).  The module (generated, loaded into chip memory) plays sample
; 31 of $9000 words with 901 in row 0 (Paula then reads chip memory
; behind the module, nothing is written).  The program finds the play
; routine's data in its own code (the library is linked behind it: the
; period table, with the four voices of 44 bytes behind it) and reads
; the start (+4) and the length (+8) of voice 0 after row 0 was played.
; The rows are played by hand (TrackPlay: one tick; speed 6, row 1 at
; the 7th tick; the CIA interrupt does not play in the emulator).
; Correct: the start moves (by the offset), the length is not 1.
; @rwe ritrackerlib
; @pass file=pt.mod|@TESTDATA@/pt23a-9xx-signed-length.mod
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
NPrint "start moved ",Peek.l(v+4)<>s," length 1 ",Peek.w(v+8)=1
FreeMem_ m,4096
End

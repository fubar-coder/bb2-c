; pt23a-repeat-word (#479) - the loop start of a sample is start + 2 x
; repeat, the doubling done as a word (ASL.W): from a repeat of 32768
; words on the loop starts 64 KB too early.  Tested with ritrackerlib
; (ciatrklib has the same code).  The module (generated, loaded into
; chip memory) plays sample 31 with a repeat from word $8000 in row 0
; (Paula then reads chip memory behind the module, nothing is written).
; The program finds the play routine's data in its own code (the library
; is linked behind it: the period table, with the four voices of 44
; bytes behind it) and reads the start (+4) and loop start (+10) of
; voice 0 after row 0 was played. The rows are played by hand
; (TrackPlay: one tick; speed 6, row 1 at the 7th tick; the CIA
; interrupt does not play in the emulator).
; Correct: the loop starts $10000 bytes behind the start.
; @rwe ritrackerlib
; @pass file=pt.mod|@TESTDATA@/pt23a-repeat-word.mod
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
NPrint "voice 0 start ",Peek.l(v+4)-s," loop ",Hex$(Peek.l(v+10)-Peek.l(v+4))
FreeMem_ m,4096
End

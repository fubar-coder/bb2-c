; trackerlib-repeat-wort (#291) - the repeat start of a sample (in words)
; is doubled as a word (ASL.w): from 32768 words (64K) on, the loop start
; wraps.  The module here (generated) plays sample 31 with a repeat from
; word $8000 in row 0 (Paula then reads chip memory behind the module,
; nothing is written).  The program finds the voice data of the player in
; its own code (the library is linked behind it: the table of the 31
; sample starts, known from the module, with the four voices of 28 bytes
; behind it) and reads the start ($4.l) and loop start ($a.l) of voice 0
; after row 0 was played (PlayModule: speed 6, one row per 6 frames).
; Correct: the loop starts $10000 bytes behind the start.
; @configs clib
; @pass file=rep.mod|@TESTDATA@/trackerlib-repeat-wort.mod
DEFTYPE .l
Function.l findmt{b.l,s0.l,s30.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=s0
      If Peek.l(a+120)=s30 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function
here:
LoadModule 0,"CORPUS:rep.mod"
d=Peek.l(Addr Module(0))
s=d+$43C+1024
PlayModule 0
VWait 10
StopModule
t=findmt{?here,s,s}
NPrint "voices found ",t<>0
v=t+124
NPrint "voice 0 start ",Peek.l(v+4)-s," loop ",Hex$(Peek.l(v+$A)-Peek.l(v+4))
End

; trackerlib-setvol-byte (#293) - effect C (set volume) writes the volume
; only to AUDxVOL (as a byte), not to the volume of the voice ($12.w),
; which a following volume slide (A) computes with.  (The byte write to
; the write-only register cannot be read back.)  The module here
; (generated) plays sample 1 (volume $20) with C10 in row 0 and slides
; down by 5 in row 1.  The program finds the voice data of the player in
; its own code (the library is linked behind it: the table of the 31
; sample starts, known from the module, with the four voices of 28 bytes
; behind it) and reads the volume of voice 0 after row 1 (PlayModule:
; speed 6, one row per 6 frames).
; Correct: the slide starts at $10 - the voice ends at $0B.
; @configs clib
; @pass file=setvol.mod|@TESTDATA@/trackerlib-setvol-byte.mod
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
LoadModule 0,"CORPUS:setvol.mod"
d=Peek.l(Addr Module(0))
s=d+$43C+1024
PlayModule 0
VWait 21
StopModule
t=findmt{?here,s,s+4}
NPrint "voices found ",t<>0
v=t+124
NPrint "voice 0 start ",Peek.l(v+4)-s," volume ",Hex$(Peek.w(v+$12) AND $FFFF)
End

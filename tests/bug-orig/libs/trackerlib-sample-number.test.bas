; trackerlib-sample-number (#295) - the sample number of a note (up to 255)
; indexes the table of the 31 sample starts without a check: sample 32
; takes its start from behind the table - the first long of voice 0, the
; note just read ($21AC0000 here), and Paula plays from there.  The module
; here (generated) plays sample 32 in row 0.  The case finds the voice
; data of the player in its own code (the library is linked behind it:
; the table of the 31 sample starts, known from the module, with the four
; voices of 28 bytes behind it) and reads the sample start of voice 0
; ($4.l) after row 0 was played (PlayModule: speed 6, one row per 6
; frames).
; Correct: a sample number above 31 sets no start outside the module (the
; start stays 0, or is one of the module's samples); the port clears the
; number when the module is started, so the note reads $01AC0000.
DEFTYPE .l
Function.l findmt{b.l,s0.l,s30.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=s0
      If Peek.l(a+120)=s30 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{sample_32}
  here:
  LoadModule 0,"CORPUS:smp.mod"
  d=Peek.l(Addr Module(0))
  l=Peek.l(Addr Module(0)+4)
  s=d+$43C+1024
  PlayModule 0
  VWait 10
  StopModule
  t=findmt{?here,s,s}
  !Require{t<>0,"voices_found"}
  v=t+124
  p=Peek.l(v+4)
  note=Peek.l(v)
  outside=0
  If p<>0 Then If p<d OR p>=d+l Then outside=1
  !CheckEq{note,$01AC0000,"voice0_note"}
  !CheckEq{outside,0,"start_outside_module"}
!EndTestCase

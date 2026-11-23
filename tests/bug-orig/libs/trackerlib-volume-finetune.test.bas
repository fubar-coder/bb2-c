; trackerlib-volume-finetune (#290) - a note with a sample takes the word
; at sample+2 of the sample header as the volume of the voice - the
; finetune byte with the volume byte.  The module here (generated) plays
; sample 1 (finetune 5, volume $30) in row 0.  The voice data of the
; player is not reachable from Blitz: the case finds it in its own
; code (the library is linked behind it) as the table of the 31 sample
; starts (mt_samplestarts, known from the module) with the four voices
; (28 bytes each) behind it, and reads the volume of voice 0 ($12.w)
; after row 0 was played (PlayModule: speed 6, one row per 6 frames).
; Correct: the volume of the voice is $0030.
DEFTYPE .l
Function.l findmt{b.l,s0.l,s30.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=s0
      If Peek.l(a+120)=s30 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{voice_volume}
  here:
  LoadModule 0,"CORPUS:vol.mod"
  d=Peek.l(Addr Module(0))
  s=d+$43C+1024
  PlayModule 0
  VWait 10
  StopModule
  t=findmt{?here,s,s+4}
  !Require{t<>0,"voices_found"}
  v=t+124
  vstart=Peek.l(v+4)-s
  vol=Peek.w(v+$12) AND $FFFF
  !CheckEq{vstart,0,"voice0_start"}
  !CheckEq{vol,$0030,"voice0_volume"}
!EndTestCase

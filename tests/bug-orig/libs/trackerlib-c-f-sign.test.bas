; trackerlib-c-f-sign (#294) - effects C and F limit their parameter
; with a signed compare: $80-$FF are not limited.  F80 makes the speed
; -128, and mt_music (signed compare with the frame counter) plays a new
; row in every frame.  The module here (generated) has F80 in row 0 and a
; row marker on channel 1 (0 in row 0, 1 in rows 1-15).  The case
; finds the voice data of the player in its own code (the library is
; linked behind it: the table of the 31 sample starts, known from the
; module, with the four voices of 28 bytes behind it) and reads the note
; of voice 1 ($0.l, parameter in its low byte) about 12 frames after
; PlayModule: row 0 comes after 6 frames.  (Effect C with $80-$FF only
; writes the write-only AUDxVOL.)
; Correct: F80 is limited to speed 31 - row 0 is still playing.
DEFTYPE .l
Function.l findmt{b.l,s0.l,s30.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=s0
      If Peek.l(a+120)=s30 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{speed_limit}
  here:
  LoadModule 0,"CORPUS:speed.mod"
  d=Peek.l(Addr Module(0))
  s=d+$43C+1024
  PlayModule 0
  VWait 12
  StopModule
  t=findmt{?here,s,s}
  !Require{t<>0,"voices_found"}
  v=t+124+28
  cmd=Peek.b(v+2) AND 255
  later=0
  If Peek.b(v+3)<>0 Then later=1
  !CheckEq{cmd,8,"voice1_command"}
  !CheckEq{later,0,"later_row"}
!EndTestCase

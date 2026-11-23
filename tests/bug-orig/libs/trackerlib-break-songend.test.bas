; trackerlib-break-songend (#292) - after a pattern break (D) or a
; position jump (B) mt_nex compares the next position with the byte a0
; points to: the song length only at the end of a pattern; after a break
; in another row a0 still points to the pattern data, so the song end is
; compared with its first byte (and the restart position read from the
; second).  The module here (generated, song length 2, restart 0) breaks
; in row 16 of its last song position (pattern 1, F01 makes it speed 1);
; position 2 behind the song end holds pattern 2.  The case finds the
; voice data of the player in its own code (the library is linked behind
; it: the table of the 31 sample starts, known from the module, with the
; four voices of 28 bytes behind it) and reads the row marker of voice 1
; ($3.b: 1 in pattern 0, 2 in pattern 1, 3 in pattern 2) about 100 frames
; after PlayModule: the break comes after 87 frames.
; Correct: after the break the song starts again at the restart position
; (pattern 0).
DEFTYPE .l
Function.l findmt{b.l,s0.l,s30.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=s0
      If Peek.l(a+120)=s30 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{after_break}
  here:
  LoadModule 0,"CORPUS:break.mod"
  d=Peek.l(Addr Module(0))
  s=d+$43C+3*1024
  PlayModule 0
  VWait 100
  StopModule
  t=findmt{?here,s,s}
  !Require{t<>0,"voices_found"}
  v=t+124+28
  cmd=Peek.b(v+2) AND 255
  pat=Peek.b(v+3)
  !CheckEq{cmd,8,"voice1_command"}
  !CheckEq{pat,1,"voice1_pattern"}
!EndTestCase

; shapeslib-loadshape-anim-interleave (#1166) - an extension, not a bug of
; the original: LoadShape of an ANIM5 takes the interleave of each ANHD as
; the ANIM specification defines it, the delta modifies the frame n
; frames back, 0 = 2 (double buffering, the default of most ANIM5 files);
; a frame with fewer frames before it is made from the first frame.  The
; original took only 1 and stopped on any other value with "Illegal ANIM
; interleave value" (its cells declare it: SetErr catches the error, the
; check no_error fails and the case ends there; extension = 1166).
; Here (generated) an ANIM of 16 x 2 pixels, one plane: frame 0 the rows
; $0102 and $0304, then 8 delta frames, each setting one byte (frame 4
; XOR $40), with the interleaves 0, 0, 0, 0, 1, 2, 3, 255; LoadShape puts
; them into the shapes 0-8 (shape 9 stays empty).
; Correct: frame 1 from 0, 2 from 0, 3 from 1, 4 from 2, 5 from 4
; (interleave 1), 6 from 4 (2), 7 from 4 (3), 8 from 0 (255: before the
; first frame); each frame is checked as "frame N WIDTH HEIGHT BYTES".
DEFTYPE .l

!TestCase{interleave}
  SetErr
    !Check{0,"no_error"}
    End
  End SetErr
  LoadShape 0,"CORPUS:anim.iff"
  ClrErr
  Restore framedata
  For s=0 To 8
    Read want$
    d=Peek.l(Addr Shape(s)+14) : s$=""
    For i=0 To 3 : s$=s$+Right$(Hex$(Peek.b(d+i) AND 255),2) : Next
    f$="frame "+Str$(s)+" "+Str$(Peek.w(Addr Shape(s)))+" "+Str$(Peek.w(Addr Shape(s)+2))+" "+s$
    !CheckStrEq{f$,want$,"frame"}
  Next
  !CheckEq{Peek.w(Addr Shape(9)),0,"shape_9_empty"}
!EndTestCase

.framedata
Data$ "frame 0 16 2 01020304","frame 1 16 2 10020304","frame 2 16 2 01200304"
Data$ "frame 3 16 2 10023004","frame 4 16 2 01200344","frame 5 16 2 50200344"
Data$ "frame 6 16 2 01206644","frame 7 16 2 01770344","frame 8 16 2 88020304"

; shapeslib-loadshape-anim-interleave (#1166) - an extension, not a bug of
; the original: LoadShape of an ANIM5 takes the interleave of each ANHD as
; the ANIM specification defines it, the delta modifies the frame n
; frames back, 0 = 2 (double buffering, the default of most ANIM5 files);
; a frame with fewer frames before it is made from the first frame.  The
; original took only 1 and stopped on any other value with "Illegal ANIM
; interleave value".
; Here (generated) an ANIM of 16 x 2 pixels, one plane: frame 0 the rows
; $0102 and $0304, then 8 delta frames, each setting one byte (frame 4
; XOR $40), with the interleaves 0, 0, 0, 0, 1, 2, 3, 255; LoadShape puts
; them into the shapes 0-8 (shape 9 stays empty).  SetErr catches the
; original's runtime error (it prints "error").
; Correct: frame 1 from 0, 2 from 0, 3 from 1, 4 from 2, 5 from 4
; (interleave 1), 6 from 4 (2), 7 from 4 (3), 8 from 0 (255: before the
; first frame).
; @fixed #1166/port
; @configs clib
; @pass file=anim.iff|@TESTDATA@/shapeslib-loadshape-anim-interleave.anim
DEFTYPE .l
SetErr
  NPrint "error"
  End
End SetErr
LoadShape 0,"CORPUS:anim.iff"
For s=0 To 8
  d=Peek.l(Addr Shape(s)+14) : s$=""
  For i=0 To 3 : s$=s$+Right$(Hex$(Peek.b(d+i) AND 255),2) : Next
  NPrint "frame ",s," ",Peek.w(Addr Shape(s))," ",Peek.w(Addr Shape(s)+2)," ",s$
Next
NPrint "shape 9 ",Peek.w(Addr Shape(9))
End

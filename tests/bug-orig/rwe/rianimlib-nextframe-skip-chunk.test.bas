; rianimlib-nextframe-skip-chunk (#1116) - RINextAnimFrame takes a chunk
; of the ANIM that is no FORM, between two frames, as the end of the
; animation.  The ANIM (16 x 16, 1 plane, AnimLoop Off) has an ANNO
; chunk between frame 2 (row 0 of column 0 gets $F0) and frame 3 (row 1
; gets $0F); the second RINextAnimFrame meets the ANNO and returns 0.
; Correct: the ANNO is skipped, frame 3 is played (-1, row 1 is $0F).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{skip_chunk}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  AnimLoop Off
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r2=RINextAnimFrame(0)
  r3=RINextAnimFrame(0)
  v=Peek.b(p+2) AND 255
  FreeMem_ a,4096
  !CheckEq{r2,-1,"frame_2"}
  !CheckEq{r3,-1,"frame_3"}
  !CheckEq{v,$0F,"frame_3_row_1"}
!EndTestCase

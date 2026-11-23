; rianimlib-anim7-short-form (#504) - ANIM7 stores its data as words or
; longs (bit 0 of the ANHD bits); the test for the short form tests the
; operation (7) instead of the bits, so every ANIM7 is decoded as long
; data.  Frame 2 of the ANIM (32 x 16, 1 plane) has short data: column 0
; (a word) gets $1234 in row 0, column 1 has no op; the data list holds
; the words $1234 and $5678.  Decoded as longs, row 0 is $12345678.
; Correct: the words are decoded, row 0 is $1234 0000.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{short_form}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,32,16,1
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  v=Peek.b(p+2) AND 255
  FreeMem_ a,4096
  !CheckEq{v,0,"short_data"}
!EndTestCase

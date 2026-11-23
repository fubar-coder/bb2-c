; rianimlib-init-fail-state (#512) - RIAnimInit stores the new ANIM's
; address and end before it checks for the FORM ILBM and its BMHD; when
; one is missing it returns 0 but keeps them, and the frame pointers of
; the animation initialised before.  A good RIAnimInit (two frames), then
; one of a FORM ANIM whose only frame has no BMHD (built by the program
; behind the file): RIAnimFrameCount counts the rejected ANIM's chunk,
; RINextAnimFrame plays the first animation's second frame.
; Correct: after the failed RIAnimInit there is no animation:
; RIAnimFrameCount and RINextAnimFrame return 0.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{init_fail_state}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  !CheckEq{r,-1,"good_init"}
  b=a+2048                        ; FORM ANIM { FORM ILBM { CMAP } }
  Poke.l b,$464F524D : Poke.l b+4,24 : Poke.l b+8,$414E494D
  Poke.l b+12,$464F524D : Poke.l b+16,12 : Poke.l b+20,$494C424D
  Poke.l b+24,$434D4150 : Poke.l b+28,0
  r=RIAnimInit(b,0,0)
  !CheckEq{r,0,"bad_init"}
  c.w=RIAnimFrameCount
  !CheckEq{c,0,"count"}
  r=RINextAnimFrame(0)
  !CheckEq{r,0,"next"}
  FreeMem_ a,4096
!EndTestCase

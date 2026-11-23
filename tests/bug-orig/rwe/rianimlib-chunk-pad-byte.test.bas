; rianimlib-chunk-pad-byte (#505) - the chunk walks step by length + 8
; without the pad byte of an odd length.  The first frame starts with an
; ANNO chunk of 5 bytes: the search for the BMHD is one byte off behind
; it, reads a garbage length and gives up; RIAnimDepth returns 0.
; Correct: the BMHD is found, RIAnimDepth returns 1.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{pad_byte}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  d.w=RIAnimDepth(a)
  FreeMem_ a,4096
  !CheckEq{d,1,"depth"}
!EndTestCase

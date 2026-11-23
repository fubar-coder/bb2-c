; rianimlib-chunk-odd-address-68000 (#1119) - behind a chunk of odd
; length whose pad byte is missing (a byte other than 0 there) the chunk
; walks take that byte as the start of the next chunk, at an odd address,
; and read its ID and length as longs: on a 68000 that is an address
; error (the session ends in a timeout); rianimlib-chunk-pad-byte shows
; the 68020 side.  The machine is the A600 (68000).  The first frame
; starts with an ANNO of 5 bytes without its pad byte, the BMHD behind it.
; Correct: RIAnimInit refuses the ANIM (0), RIAnimDepth finds no BMHD (0).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{odd_address}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  d.w=RIAnimDepth(a)
  !CheckEq{r,0,"init"}
  !CheckEq{d,0,"depth"}
  FreeMem_ a,4096
!EndTestCase

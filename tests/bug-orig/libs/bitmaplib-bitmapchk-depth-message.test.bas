; bitmaplib-bitmapchk-depth-message (#1040) - the error check of BitMap
; (_bitmapchk) refuses a depth of 0 or more than 8 with error number 1,
; which the debugger shows as "No currently used object!": a misleading
; message for a wrong depth.
; Here with runtime error checking: BitMap with depth 2 (no error), then
; with depth 0.
; Correct: the runtime error names the depth: "Illegal BitMap Depth" (the
; text of CludgeBitMap's check).  The message is the tested behaviour, so
; the error is not announced with !ExpectError (an announced error that
; comes with another text fails the case in every cell): the requester is
; asserted in the configuration.
!TestCase{depth_zero}
  BitMap 0,64,10,2
  !CheckEq{Peek.b(Addr BitMap(0)+5),2,"depth_2"}
  BitMap 1,64,10,0
  !Check{0,"after"}
!EndTestCase

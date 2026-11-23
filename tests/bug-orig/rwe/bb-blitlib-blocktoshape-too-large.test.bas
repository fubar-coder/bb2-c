; bb-blitlib-blocktoshape-too-large (#425) - BlockToShape copies a shape
; with one blit of height x depth lines; the blitter's height field has 10
; bits (0 = 1024 lines).  A 16x256 shape of 5 planes has 1280 lines: the
; size word (1280 << 6) loses its top bits and the blit copies 256 lines.
; The original's error check tests only the destination and the sizes, so
; also with runtime error checking the copy is silently incomplete.
; Here with runtime error checking.
; Correct: the error check refuses the shape: "Shape too large for
; BlockToShape!".
!TestCase{too_large}
  BitMap 0,320,256,5
  Boxf 0,0,15,255,31
  GetaShape 0,0,0,16,256
  GetaShape 1,16,0,16,256
  OwnBlitter_
  !ExpectError{"Shape too large for BlockToShape!","refused"}
  BlockToShape 0,1
  !Check{0,"copied"}
  WaitBlit_
  DisownBlitter_
!EndTestCase

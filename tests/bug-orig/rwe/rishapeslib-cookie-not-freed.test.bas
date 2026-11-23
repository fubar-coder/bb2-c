; rishapeslib-cookie-not-freed (#810) - LEInitShape and readshapes
; (LELoadShapes, LECludgeShapes) free the data of a used shape, then test
; its cookie with MOVE.l 18(a2),a1:BEQ - a MOVEA sets no flags, the BEQ
; tests the CLR before it and is always taken: the cookie (one plane of
; chip memory) is never freed.  Shape 0 (64 x 64 pixels, cookie 512
; bytes) is made again 50 times with LEInitShape, then saved and loaded
; again 50 times with LELoadShapes; the free chip memory is compared
; before and after each loop.
; Correct: no chip memory lost; the original loses 50 cookies each.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{cookie_not_freed}
  LEInitShape 0,64,64,1
  a=AvailMem_(2)
  For i=1 To 50 : LEInitShape 0,64,64,1 : Next
  b=AvailMem_(2)
  lost1=a-b
  LESaveShapes 0,0,"CORPUS:c.le"
  LELoadShapes 0,0,"CORPUS:c.le"
  a=AvailMem_(2)
  For i=1 To 50 : LELoadShapes 0,0,"CORPUS:c.le" : Next
  b=AvailMem_(2)
  lost2=a-b
  !Check{lost1<=10000,"leinitshape_kept"}
  !Check{lost2<=10000,"leloadshapes_kept"}
!EndTestCase

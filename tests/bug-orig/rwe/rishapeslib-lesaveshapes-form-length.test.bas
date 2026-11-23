; rishapeslib-lesaveshapes-form-length (#816) - LESaveShapes patches the
; FORM length with the SDAT size + the CMAP size + 40, but the FORM
; header, LESH and the SHHD chunk are 40 bytes with the FORM header's 8:
; the length is 8 too large.  The CMAP size is kept from the last save
; with a palette, so a save without one adds it again.  Shape 0 is saved
; with a palette of 4 colours, then without; the length of each file
; (Read) and its FORM length (offset 4) are compared as file size - 8 and
; FORM length.
; Correct: both equal (the file size - 8); the original writes 8 more,
; and 8 + 20 more without the palette.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{form_length}
  InitPalette 0,4
  LEInitShape 0,16,8,1
  m=AllocMem_(1024,$10001)
  !Require{m<>0,"memory"}
  LESaveShapes 0,0,"CORPUS:p.le",0
  n$="CORPUS:p.le"
  f=Open_(&n$,1005) : r=Read_(f,m,1024) : Close_ f
  s1=r-8 : l1=Peek.l(m+4)
  LESaveShapes 0,0,"CORPUS:q.le"
  n$="CORPUS:q.le"
  f=Open_(&n$,1005) : r=Read_(f,m,1024) : Close_ f
  s2=r-8 : l2=Peek.l(m+4)
  FreeMem_ m,1024
  !CheckEq{s1,88,"palette_file_size"}
  !CheckEq{l1,88,"palette_form_length"}
  !CheckEq{s2,68,"plain_file_size"}
  !CheckEq{l2,68,"plain_form_length"}
!EndTestCase

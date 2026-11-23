; rishapeslib-cludged-shape-pop (#807) - readshapes (LELoadShapes,
; LECludgeShapes) pushes a0 around the freeing of a used target shape but
; skips the push for a shape linked into memory ("cL"), not the pop: it
; takes its own return address as the data pointer and returns with the
; stack one long short - LELoadShapes returns past its cleanup, the file
; stays open.  The LE file (generated: one uncompressed shape) is read
; into chip memory and linked to shape 0 with LECludgeShapes, then
; loaded into shape 0 with LELoadShapes.  The program runs through
; inject (runwrap=, inject inuse: is the file still in use afterwards).
; Correct: LELoadShapes closes the file.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{cludged_shape_pop}
  n$="CORPUS:s.le"
  f=Open_(&n$,1005)
  !Require{f<>0,"file"}
  m=AllocMem_(1024,$10002)
  !Require{m<>0,"memory"}
  r=Read_(f,m,1024)
  Close_ f
  LECludgeShapes 0,0,m,0
  w=ShapeWidth(0)
  LELoadShapes 0,0,"CORPUS:s.le"
  !CheckEq{w,16,"linked"}
  !Check{1,"loaded"}
!EndTestCase

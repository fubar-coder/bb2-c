; rishapeslib-cludged-shape-pop (#807) - readshapes (LELoadShapes,
; LECludgeShapes) pushes a0 around the freeing of a used target shape but
; skips the push for a shape linked into memory ("cL"), not the pop: it
; takes its own return address as the data pointer and returns with the
; stack one long short - LELoadShapes returns past its cleanup, the file
; stays open.  The victim (rishapeslib-cludged-shape-pop.test/victim.bas) reads
; the LE file (generated: one uncompressed shape) into chip memory, links it
; to shape 0 with LECludgeShapes and loads it into shape 0 with LELoadShapes;
; the job line of the test runs it from the editor and inject reports whether
; the file is still in use afterwards.
; Correct: LELoadShapes closes the file.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{cludged_shape_pop}
  !Check{1,"driver"}
!EndTestCase

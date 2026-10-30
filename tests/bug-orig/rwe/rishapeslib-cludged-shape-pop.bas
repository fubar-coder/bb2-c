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
; @rwe rishapeslib ripacklib riamosfunclib
; @pass file=s.le|@TESTDATA@/rishapeslib-cludged-shape-pop.le
; @pass runwrap=@INJECT@|inuse=CORPUS:s.le
DEFTYPE .l
n$="CORPUS:s.le"
f=Open_(&n$,1005)
If f=0 Then NPrint "no file" : End
m=AllocMem_(1024,$10002)
r=Read_(f,m,1024)
Close_ f
LECludgeShapes 0,0,m,0
NPrint "linked ",ShapeWidth(0)
LELoadShapes 0,0,"CORPUS:s.le"
NPrint "loaded"
End

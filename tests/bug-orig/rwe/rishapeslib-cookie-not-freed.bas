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
; @fixed #810
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
LEInitShape 0,64,64,1
a=AvailMem_(2)
For i=1 To 50 : LEInitShape 0,64,64,1 : Next
b=AvailMem_(2)
If a-b>10000 Then NPrint "LEInitShape: chip memory lost" Else NPrint "LEInitShape: kept"
LESaveShapes 0,0,"RAM:c.le"
LELoadShapes 0,0,"RAM:c.le"
a=AvailMem_(2)
For i=1 To 50 : LELoadShapes 0,0,"RAM:c.le" : Next
b=AvailMem_(2)
If a-b>10000 Then NPrint "LELoadShapes: chip memory lost" Else NPrint "LELoadShapes: kept"
End

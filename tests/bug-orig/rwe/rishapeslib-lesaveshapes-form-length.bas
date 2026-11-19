; rishapeslib-lesaveshapes-form-length (#816) - LESaveShapes patches the
; FORM length with the SDAT size + the CMAP size + 40, but the FORM
; header, LESH and the SHHD chunk are 40 bytes with the FORM header's 8:
; the length is 8 too large.  The CMAP size is kept from the last save
; with a palette, so a save without one adds it again.  Shape 0 is saved
; with a palette of 4 colours, then without; the length of each file
; (Read) and its FORM length (offset 4) are printed as file size - 8 and
; FORM length.
; Correct: both equal (the file size - 8); the original writes 8 more,
; and 8 + 20 more without the palette.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
; @fixed #816
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
InitPalette 0,4
LEInitShape 0,16,8,1
m=AllocMem_(1024,$10001)
If m=0 Then NPrint "no memory" : End
LESaveShapes 0,0,"RAM:p.le",0
n$="RAM:p.le"
f=Open_(&n$,1005) : r=Read_(f,m,1024) : Close_ f
NPrint "with palette: ",r-8," ",Peek.l(m+4)
LESaveShapes 0,0,"RAM:q.le"
n$="RAM:q.le"
f=Open_(&n$,1005) : r=Read_(f,m,1024) : Close_ f
NPrint "without: ",r-8," ",Peek.l(m+4)
FreeMem_ m,1024
End

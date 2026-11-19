; rishapeslib-lesaveshapes-count-upper-word (#815) - LESaveShapes computes
; the shape count of the SHHD chunk as last - first with a word
; subtraction into the long it stores: the upper word of d1 stays.
; Compiled code leaves one there when the last shape is a long
; expression: a=$50009 passes shape 9 with 5 in the upper word.  Shapes
; 0..9 are saved so and the count is read from the file (offset 20).
; Correct: the count 9 (the shapes - 1); the original writes $50009.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
; @fixed #815
; @rwe rishapeslib ripacklib riamosfunclib
DEFTYPE .l
For i=0 To 9 : LEInitShape i,16,8,1 : Next
a=$50009
LESaveShapes 0,a,"RAM:w.le"
m=AllocMem_(1024,$10001)
If m=0 Then NPrint "no memory" : End
n$="RAM:w.le"
f=Open_(&n$,1005)
If f=0 Then NPrint "no file" : End
r=Read_(f,m,1024)
Close_ f
NPrint "count ",Peek.l(m+20)
FreeMem_ m,1024
End

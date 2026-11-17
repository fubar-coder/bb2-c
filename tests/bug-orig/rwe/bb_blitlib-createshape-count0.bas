; bb_blitlib-createshape-count0 (#428) - CreateShape counts its shape
; objects with SUBQ.w #1 / DBRA: a count of 0 makes 65536 objects of 32
; bytes, 2 MB written from shape 1 on, far past the shape table.  Shape 2
; then is a copy of shape 0 (width 16).  The original has no error check
; for CreateShape, not even with runtime error checking.  With 2 MB of
; foreign memory overwritten the program cannot end safely (sometimes the
; system crashes, sometimes not): in that case it waits for the session's
; timeout.
; Here with runtime error checking.
; Correct: the error check refuses the count 0: "CreateShape count must
; be at least 1!", nothing is written.
; @fixed #428
; @rwe bb_blitlib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,64,1
Boxf 0,0,15,15,1
GetaShape 0,0,0,16,16
m=AllocMem_(256,$10002)
If m=0 Then NPrint "no memory" : End
n=0
NPrint "before"
CreateShape 1,n,m,0
w=Peek.w(Addr Shape(2))
NPrint "shape 2 width ",w
If w<>0
  Repeat : Delay_ 50 : Forever
EndIf
FreeMem_ m,256
End

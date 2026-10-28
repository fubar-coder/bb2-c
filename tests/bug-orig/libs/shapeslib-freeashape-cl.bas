; shapeslib-freeashape-cl (#269) - freeashape clears +30 only when it
; does not hold "cL": after freeing a DecodeShapes shape (data used in
; place below 2 MB) the mark stays in the object, InitShape does not write
; +30, and the data of the next shape in that object is never freed.
; Here: shape 1 from DecodeShapes in CHIP memory, Free Shape 1, then
; InitShape 1 and Free Shape 1; the free CHIP memory (sum of mh_Free of
; the CHIP MemHeaders) before the InitShape and after the Free shows the
; leak.
; Correct: the mark goes with the freed shape (+30 = 0) and Free Shape
; returns the data of the new shape (leak 0).
; @configs clib
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function
InitShape 0,32,16,2
SaveShapes 0,0,"RAM:bo269.shp"
If ReadFile(0,"RAM:bo269.shp")
  l=Lof(0) : m=AllocMem(l+64,$10002) : ReadMem 0,m,l : CloseFile 0
  DecodeShapes 1,1,m
  NPrint "decoded ",Hex$(Peek.w(Addr Shape(1)+30) AND $FFFF)
  Free Shape 1
  NPrint "freed ",Hex$(Peek.w(Addr Shape(1)+30) AND $FFFF)
  f0=chipfree{}
  InitShape 1,32,16,2
  Free Shape 1
  f1=chipfree{}
  NPrint "leak ",f0-f1
EndIf
KillFile "RAM:bo269.shp"
End

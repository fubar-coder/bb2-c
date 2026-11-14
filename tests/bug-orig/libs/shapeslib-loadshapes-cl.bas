; shapeslib-loadshapes-cl (#267) - LoadShapes reads the 32 byte block of
; each shape from the file as it is and clears only the cookie (+18): +30
; comes from the file.  SaveShapes writes the block unchanged, so the file
; of a DecodeShapes shape (data used in place below 2 MB, "cL" in +30)
; holds "cL", and the shape LoadShapes allocates from it is never freed.
; DecodeShapes of such a file in FAST memory copies the block the same
; way before it copies the data to CHIP memory.
; Here: shape 1 from DecodeShapes in CHIP memory, saved; the file loaded
; into shape 2 with LoadShapes and then Free Shape 2; the same file in
; FAST memory decoded into shape 3 and Free Shape 3.  The free CHIP
; memory (sum of mh_Free of the CHIP MemHeaders) before the load and
; after the Free shows the leak.
; Correct: the shapes own their data (+30 = 0) and Free Shape returns it
; (leak 0).
; @fixed #267
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
SaveShapes 0,0,"RAM:bo267.shp"
If ReadFile(0,"RAM:bo267.shp")
  l=Lof(0) : m=AllocMem(l+64,$10002) : ReadMem 0,m,l : CloseFile 0
  DecodeShapes 1,1,m
  NPrint "decoded ",Hex$(Peek.w(Addr Shape(1)+30) AND $FFFF)
  SaveShapes 1,1,"RAM:bo267b.shp"
  f0=chipfree{}
  LoadShapes 2,2,"RAM:bo267b.shp"
  k=Peek.w(Addr Shape(2)+30) AND $FFFF
  Free Shape 2
  f1=chipfree{}
  NPrint "loaded ",Hex$(k)
  NPrint "leak ",f0-f1
  If ReadFile(0,"RAM:bo267b.shp")
    l=Lof(0) : n=AllocMem(l+64,$10004) : ReadMem 0,n,l : CloseFile 0
    f0=chipfree{}
    DecodeShapes 3,3,n
    k=Peek.w(Addr Shape(3)+30) AND $FFFF
    Free Shape 3
    f1=chipfree{}
    NPrint "fast ",Hex$(k)
    NPrint "fast leak ",f0-f1
  EndIf
EndIf
KillFile "RAM:bo267.shp"
KillFile "RAM:bo267b.shp"
End

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
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function

!TestCase{loadshapes_leak}
  InitShape 0,32,16,2
  SaveShapes 0,0,"CORPUS:bo267.shp"
  ok=ReadFile(0,"CORPUS:bo267.shp")
  !Require{ok,"file_read"}
  l=Lof(0) : m=AllocMem(l+64,$10002) : ReadMem 0,m,l : CloseFile 0
  DecodeShapes 1,1,m
  !CheckStrEq{Hex$(Peek.w(Addr Shape(1)+30) AND $FFFF),"0000634C","decoded"}
  SaveShapes 1,1,"CORPUS:bo267b.shp"
  f0=chipfree{}
  LoadShapes 2,2,"CORPUS:bo267b.shp"
  k=Peek.w(Addr Shape(2)+30) AND $FFFF
  Free Shape 2
  f1=chipfree{}
  !CheckStrEq{Hex$(k),"00000000","loaded"}
  !CheckEq{f0-f1,0,"leak"}
  ok=ReadFile(0,"CORPUS:bo267b.shp")
  !Require{ok,"file_read_2"}
  l=Lof(0) : n=AllocMem(l+64,$10004) : ReadMem 0,n,l : CloseFile 0
  f0=chipfree{}
  DecodeShapes 3,3,n
  k=Peek.w(Addr Shape(3)+30) AND $FFFF
  Free Shape 3
  f1=chipfree{}
  !CheckStrEq{Hex$(k),"00000000","fast"}
  !CheckEq{f0-f1,0,"fast_leak"}
  KillFile "CORPUS:bo267.shp"
  KillFile "CORPUS:bo267b.shp"
!EndTestCase

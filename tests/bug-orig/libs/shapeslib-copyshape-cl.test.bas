; shapeslib-copyshape-cl (#268) - CopyShape (docopyshape2) copies the
; whole 32 byte block of the source, +30 included, and then allocates
; data of its own for the copy: the copy of a DecodeShapes shape (data
; used in place below 2 MB, "cL" in +30) carries "cL" too, and freeashape
; never frees its data.
; Here: shape 1 from DecodeShapes in CHIP memory, CopyShape 1,2, Free
; Shape 2; the free CHIP memory (sum of mh_Free of the CHIP MemHeaders)
; before CopyShape and after the Free shows the leak.
; Correct: the copy owns its data and Free Shape returns it (leak 0).
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function

!TestCase{copy_leak}
  InitShape 0,32,16,2
  SaveShapes 0,0,"CORPUS:bo268.shp"
  ok=ReadFile(0,"CORPUS:bo268.shp")
  !Require{ok,"file_read"}
  l=Lof(0) : m=AllocMem(l+64,$10002) : ReadMem 0,m,l : CloseFile 0
  DecodeShapes 1,1,m
  !CheckStrEq{Hex$(Peek.w(Addr Shape(1)+30) AND $FFFF),"0000634C","decoded"}
  f0=chipfree{}
  CopyShape 1,2
  Free Shape 2
  f1=chipfree{}
  !CheckEq{f0-f1,0,"leak"}
  KillFile "CORPUS:bo268.shp"
!EndTestCase

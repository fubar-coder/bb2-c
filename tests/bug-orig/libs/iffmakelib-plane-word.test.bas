; iffmakelib-plane-word (#180) - SaveBitmap writes the byte Depth at
; BitMap+5 as the BMHD's nPlanes, but the plane loop of the BODY compares
; with the word at +4, Flags * 256 + Depth: a bitmap whose Flags byte is
; not 0 gets Flags * 256 + Depth planes per row.
; Here: bitmap 0 is redirected to a block of the program's own (the
; pointers in the globals that hold the address of bitmap 0 are replaced
; for the call, as in rwe/rifxlib-depth-word-at-4): BytesPerRow 2, Rows 2,
; Flags 1 (BMF_CLEAR), Depth 1, and 257 plane pointers to one zeroed
; buffer, so the loop reads no pointer outside the block.  The saved file
; is read back: nPlanes of the BMHD and the length of the BODY.
; Correct: one plane per row, a BODY of 2 rows of 3 bytes (a literal
; run of the row's 2 bytes each).  The original writes 257 planes per
; row.
DEFTYPE .l

!TestCase{planes_per_row}
  Dim h(15)
  BitMap 0,16,2,1
  v=0 : a=&v
  ob=Addr BitMap(0)+1                 ; +1: not found by the scan below
  bs=8+257*4
  buf=AllocMem_(bs,$10001)
  pl=AllocMem_(64,$10001)
  fb=AllocMem_(4096,$10001)
  !Require{buf<>0 AND pl<>0 AND fb<>0,"memory"}
  For i=0 To 256 : Poke.l buf+8+i*4,pl : Next
  nh=0
  For i=-256 To 256
    p=a+i*4
    If Peek.l(p)=ob-1 AND nh<16 Then h(nh)=p : nh=nh+1 : Poke.l p,buf
  Next
  If Addr BitMap(0)<>buf
    For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  EndIf
  !Require{Addr BitMap(0)=buf,"redirected"}
  Poke.w buf,2 : Poke.w buf+2,2 : Poke.b buf+4,1 : Poke.b buf+5,1 : Poke.w buf+6,0
  SaveBitmap 0,"CORPUS:iffmakelib-plane-word.iff"
  For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  l=0
  ok=ReadFile(0,"CORPUS:iffmakelib-plane-word.iff")
  !Require{ok,"file_written"}
  l=Lof(0)
  If l>4096 Then l=4096
  ReadMem 0,fb,l
  CloseFile 0
  np=-1 : bl=-1
  For i=0 To l-8 Step 2
    If Peek.l(fb+i)=$424D4844 Then np=Peek.b(fb+i+16) AND 255
    If Peek.l(fb+i)=$424F4459 Then bl=Peek.l(fb+i+4)
  Next
  !CheckEq{np,1,"nplanes"}
  !CheckEq{bl,6,"body_length"}
  FreeMem_ buf,bs
  FreeMem_ pl,64
  FreeMem_ fb,4096
!EndTestCase

; ilbmifflib-plane-word (#188) - LoadBitMap (ilbmbitmap) stores a plane
; of the ILBM while its number is below the word at BitMap+4, Flags * 256
; + Depth, instead of the byte Depth: into a bitmap whose Flags byte is
; not 0 it writes the ILBM's planes beyond the bitmap's depth through the
; plane pointers behind it.
; Here: bitmap 0 is redirected to a block of the program's own (the
; pointers in the globals that hold the address of bitmap 0 are replaced
; for the call, as in rwe/rifxlib-depth-word-at-4): BytesPerRow 2, Rows 1,
; Flags 1 (BMF_CLEAR), Depth 1, plane 0 in buffer A, the pointer behind
; it (where a second plane would be) to buffer B.  The ILBM (16 x 1, two
; planes: A5 5A, C3 3C) is loaded into it.
; Correct: only plane 0 is stored (A A55A), B stays 0.  The original also
; writes plane 1 into B.
DEFTYPE .l

!TestCase{second_plane}
  Dim h(15)
  BitMap 0,16,1,1
  v=0 : a=&v
  ob=Addr BitMap(0)+1                 ; +1: not found by the scan below
  buf=AllocMem_(64,$10001)
  sa=AllocMem_(64,$10001)
  sb=AllocMem_(64,$10001)
  !Require{buf<>0 AND sa<>0 AND sb<>0,"memory"}
  Poke.l buf+8,sa
  Poke.l buf+12,sb
  nh=0
  For i=-256 To 256
    p=a+i*4
    If Peek.l(p)=ob-1 AND nh<16 Then h(nh)=p : nh=nh+1 : Poke.l p,buf
  Next
  If Addr BitMap(0)<>buf
    For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  EndIf
  !Require{Addr BitMap(0)=buf,"redirected"}
  Poke.w buf,2 : Poke.w buf+2,1 : Poke.b buf+4,1 : Poke.b buf+5,1 : Poke.w buf+6,0
  LoadBitMap 0,"CORPUS:p.iff"
  For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  !CheckEq{Peek.w(sa) AND $FFFF,$A55A,"plane_a"}
  !CheckEq{Peek.w(sb) AND $FFFF,0,"plane_b"}
  FreeMem_ buf,64
  FreeMem_ sa,64
  FreeMem_ sb,64
!EndTestCase

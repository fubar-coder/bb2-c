; ilbmifflib-decode-all-planes (#190) - DecodeILBM writes as many planes,
; rows and bytes per row as the ILBM has, through the bitmap's plane
; pointers and with its BytesPerRow as the row step, whatever the bitmap's
; size: the pointers behind its depth are used, and the rows run past the
; end of the planes.
; Here two ILBMs built in memory, each decoded into a bitmap whose plane
; pointers the program points at buffers of its own, filled with $EE (the
; pointers are put back before the case ends):
; a) 16x1, 3 planes, uncompressed ($1111, $2222, $3333) into a 16x1 bitmap
;    of 2 planes, its plane pointer 2 (behind the depth) at buffer c;
; b) 32x3, 1 plane, ByteRun1 (rows 01-04, 11-14, 21-24) into a 16x2 bitmap,
;    its plane 0 at buffer d (8 bytes: the 2 rows of 2 bytes, then 4
;    bytes behind them).
; Correct: only what the bitmap has is stored - c stays EEEE, d is
; 0102 1112 EEEE EEEE.
DEFTYPE .l
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement
Statement bmhd{w.l,h.l,d.l,c.l}
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{w} : pw{h} : pw{0} : pw{0} : pb{d} : pb{0} : pb{c} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{w} : pw{h}
End Statement
Function$ hx{a.l,n.l}
  s$=""
  For i=0 To n-1
    If i>0 AND (i AND 1)=0 Then s$=s$+" "
    s$=s$+Right$(Hex$(Peek.b(a+i) AND 255),2)
  Next
  Function Return s$
End Function

!TestCase{extra_plane}
  m=AllocMem(256,65536)
  c=AllocMem(8,65536)
  !Require{m<>0 AND c<>0,"memory"}
  For i=0 To 7 : Poke.b c+i,$EE : Next
  p=0 : bmhd{16,1,3,0}
  pl{$424F4459} : pl{6} : pw{$1111} : pw{$2222} : pw{$3333}
  Poke.l m+4,p-8
  BitMap 0,16,1,2
  a=Addr BitMap(0)
  o=Peek.l(a+16) : Poke.l a+16,c
  DecodeILBM 0,m
  Poke.l a+16,o
  !CheckStrEq{hx{Peek.l(a+8),2},"1111","plane_0"}
  !CheckStrEq{hx{Peek.l(a+12),2},"2222","plane_1"}
  !CheckStrEq{hx{c,2},"EEEE","buffer_behind_depth"}
  FreeMem c,8 : FreeMem m,256
!EndTestCase

!TestCase{row_overrun}
  m=AllocMem(256,65536)
  d=AllocMem(8,65536)
  !Require{m<>0 AND d<>0,"memory"}
  For i=0 To 7 : Poke.b d+i,$EE : Next
  p=0 : bmhd{32,3,1,1}
  pl{$424F4459} : pl{15}
  For r=0 To 2
    pb{3} : For i=1 To 4 : pb{r*16+i} : Next
  Next
  pb{0}
  Poke.l m+4,p-8
  BitMap 1,16,2,1
  a=Addr BitMap(1)
  o=Peek.l(a+8) : Poke.l a+8,d
  DecodeILBM 1,m
  Poke.l a+8,o
  !CheckStrEq{hx{d,8},"0102 1112 EEEE EEEE","buffer_d"}
  FreeMem d,8 : FreeMem m,256
!EndTestCase

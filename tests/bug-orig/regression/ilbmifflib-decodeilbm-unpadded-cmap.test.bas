; ilbmifflib-decodeilbm-unpadded-cmap (#189) - a guard of the fix of
; #189, not a bug of the original: DecodeILBM skips the pad byte of a
; chunk of odd length, but the original SaveBitmap (#184) wrote an odd
; CMAP without it, the BODY right behind. A byte other than 0 where the
; pad byte should be is taken as the start of the next chunk, so such a
; file stays readable.
; Here an ILBM in memory, 16x1, 1 plane, uncompressed, with a CMAP of one
; colour (3 bytes) and no pad byte: the BODY starts at an odd offset (the
; 68020 of the A1200 reads it; a 68000 would fault, as in the original).
; Correct (what the original does): the row $A55A is in the bitmap.

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

!TestCase{unpadded_cmap}
  m=AllocMem(256,65536) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{16} : pw{1} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{16} : pw{1}
  pl{$434D4150} : pl{3} : pb{$11} : pb{$22} : pb{$33}
  pl{$424F4459} : pl{2} : pb{$A5} : pb{$5A}
  Poke.l m+4,p-8
  BitMap 0,16,1,1
  DecodeILBM 0,m
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$A55A,"row"}
  FreeMem m,256
!EndTestCase

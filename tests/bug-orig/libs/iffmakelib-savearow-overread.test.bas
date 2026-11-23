; iffmakelib-savearow-overread (#183) - the ByteRun1 packer savearow
; ends a literal run when the next two bytes equal the last one, and at
; the last byte of the row the second of them is the byte behind the row
; (CMP.b 1(a2,d0),d2): the first byte of the next row, or memory behind
; the bitplane for the last row. If it matches, the literal ends one byte
; early and the last bytes are packed as a run of their own.
; Here a bitmap of 32x2 pixels, 1 plane: row 0 is 01 02 05 05, row 1
; starts with 05 (05 06 07 08). SaveBitmap to CORPUS:, then the BODY of
; the file (offset 48, after FORM, ILBM and BMHD).
; Correct: the packing of a row depends only on the row: row 0 is packed
; like any row of 4 different bytes, 02 01 02 05 00 05.
DEFTYPE .l

!TestCase{pack_rows}
  BitMap 0,32,2,1
  b=Peek.l(Addr BitMap(0)+8)
  Poke.l b,$01020505 : Poke.l b+4,$05060708
  SaveBitmap 0,"CORPUS:bo183.iff"
  m=AllocMem(64,65536)
  !Require{m<>0,"memory"}
  ok=ReadFile(0,"CORPUS:bo183.iff")
  !Require{ok,"file_written"}
  ReadMem 0,m,64
  CloseFile 0
  KillFile "CORPUS:bo183.iff"
  n=Peek.l(m+44)
  s$=""
  For i=0 To n-1
    If i>0 Then s$=s$+" "
    s$=s$+Right$(Hex$(Peek.b(m+48+i) AND 255),2)
  Next
  !CheckStrEq{Peek$(m+40),"BODY","chunk_id"}
  !CheckEq{n,12,"body_length"}
  !CheckStrEq{s$,"02 01 02 05 00 05 02 05 06 07 00 08","body_bytes"}
  FreeMem m,64
!EndTestCase

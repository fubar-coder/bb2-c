; ilbmifflib-decodeilbm-oddchunk (#189) - DecodeILBM walks the chunks in
; memory by their length without the pad byte of an odd length.  The ILBM
; here (generated, read into memory) has an ANNO of 5 bytes between BMHD
; and BODY: the search for BODY goes on one byte too early and reads the
; next chunk ("(c) ") shifted, its length as $20xxxxxx - behind the end of
; the FORM: runtime error "Can't find BODY" (caught with SetErr).
; Correct: the pad byte is skipped, the BODY is found - the row of the
; picture ($A55A) is in the plane of the BitMap.
DEFTYPE .l

!TestCase{odd_anno}
  Dim m.l(64)
  ok=ReadFile(0,"CORPUS:pic.ilbm")
  !Require{ok,"file_read"}
  ReadMem 0,&m(0),256
  CloseFile 0
  BitMap 0,16,1,1
  SetErr
    !Check{0,"body_found"}
    End
  End SetErr
  DecodeILBM 0,&m(0)
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$A55A,"row"}
!EndTestCase

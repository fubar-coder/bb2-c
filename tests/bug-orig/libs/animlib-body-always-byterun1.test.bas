; animlib-body-always-byterun1 (#397) - InitAnim decodes the BODY of an
; ANIM as ByteRun1 whatever the BMHD's compression byte says, and does
; not clip a run at the end of the row (it writes behind the row).
; Here (both ANIMs generated, one plane, a bitmap of 32 x 2 pixels, 4
; bytes per row):
; 1. an uncompressed BODY (compression 0) of 32 x 2 pixels: rows
;    $FDAAFDBB, $FCCC0102 - read as ByteRun1 the runs 4 x $AA, 4 x $BB;
; 2. a ByteRun1 BODY of 16 x 2 pixels whose first run (4 x $11) is
;    longer than the row (2 bytes): the bytes 2-3 of row 0 lie beyond the
;    anim's width.
; Correct: 1. the rows as stored; 2. row 0 $1111 and nothing behind it
; (bytes 2-3 stay 0), row 1 $2222.
DEFTYPE .l

!TestCase{raw_body}
  BitMap 0,32,2,1
  p=Peek.l(Addr BitMap(0)+8)
  LoadAnim 0,"CORPUS:raw.anim"
  InitAnim 0
  !CheckStrEq{Hex$(Peek.l(p)),"FDAAFDBB","row_0"}
  !CheckStrEq{Hex$(Peek.l(p+4)),"FCCC0102","row_1"}
!EndTestCase

!TestCase{run_past_row}
  BitMap 0,32,2,1
  p=Peek.l(Addr BitMap(0)+8)
  Cls
  LoadAnim 1,"CORPUS:run.anim"
  InitAnim 1
  !CheckStrEq{Hex$(Peek.l(p)),"11110000","row_0"}
  !CheckStrEq{Hex$(Peek.l(p+4)),"22220000","row_1"}
!EndTestCase

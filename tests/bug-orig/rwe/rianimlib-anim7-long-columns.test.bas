; rianimlib-anim7-long-columns (#1115) - the ANIM7 long-data decoder
; takes the number of long columns as width >> 5: a width below 32 gives
; 0 columns, which its DBRA loop runs as 65536.  The ANIM is 16 x 16
; (one word per row), the delta of frame 2 has no op in column 0 and a
; run of $FFFFFFFF in column 1; the 65534 columns behind read op counts
; of 0 from the cleared buffer (no writes).  With 65536 columns, column 1
; is decoded: rows 2 and 3 get $FFFF.
; Correct (either reading of a row with an odd number of words: no long
; column or one): column 1 is not decoded, rows 2 and 3 stay 0.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{long_columns}
  a=AllocMem_(70000,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,16,16,1
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  v=Peek.l(p+4)
  FreeMem_ a,70000
  !CheckEq{v,0,"column_1"}
!EndTestCase

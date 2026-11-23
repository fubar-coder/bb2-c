; rianimlib-anim7-long-columns-wide (#1115) - the ANIM7 long-data decoder
; takes the number of long columns as width >> 5 with a signed word shift
; (ASR.w): a width of 32768 gives -1024 - 1, which its DBRA loop runs as
; 64512 columns instead of 1024.  The ANIM is 32768 x 2 (1024 longs per
; row), the delta of frame 2 has a run of $11111111 in column 1023 (the
; last long of row 0) and one of $22222222 in column 1024; the columns
; behind have no op (no writes).  With 64512 columns, column 1024 is
; decoded: the first long of row 1 gets $22222222.
; Correct: 1024 columns; column 1023 is decoded, column 1024 is not, the
; first long of row 1 stays 0.
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{long_columns_wide}
  a=AllocMem_(80000,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,80000)
  Close_ f
  BitMap 0,32768,2,1
  r=RIAnimInit(a,0,0)
  !Require{r<>0,"init"}
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  u=Peek.l(p+4092)
  v=Peek.l(p+4096)
  FreeMem_ a,80000
  !CheckEq{u,$11111111,"column_1023"}
  !CheckEq{v,0,"column_1024"}
!EndTestCase

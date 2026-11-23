; rianimlib-anim7-long-columns-rest (#1115) - the ANIM7 long-data decoder
; takes the number of long columns as width >> 5: the rest of a width
; that is no multiple of 32 is dropped, also when the word-aligned row
; has an even number of words and so a last long column of its own.  The
; ANIM is 56 x 16 (4 words, 2 longs per row), the delta of frame 2 has
; no op in column 0 and a run of $FFFFFFFF in column 1.  With 56 >> 5 = 1
; column, column 1 is not decoded.
; Correct: the long columns of the word-aligned row, 2; column 1 is
; decoded, the second long of row 0 gets $FFFFFFFF.  (Only the last word
; of a row with an odd number of words has no long column of its own.)
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{long_columns_rest}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,56,16,1
  r=RIAnimInit(a,0,0)
  !Require{r<>0,"init"}
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  v=Peek.l(p+4)
  FreeMem_ a,4096
  !CheckEq{v,$FFFFFFFF,"column_1"}
!EndTestCase

; ricopperfxlib-colsplit-line0-wait (#588) - before each line of a colour
; split DoColSplit tests only the low byte of the line and, if it is 0,
; writes the line 256 wait $FFE1FFFE.  A split from line 0 gets it as
; well: its first line waits for line 256.  The custom space is the
; program's own memory (CopperResetAdr).
; Correct: a split from line 0 starts with the WAIT of line 0
; ($0001,$FFEE); a split from line 250 still has the line 256 wait
; before its seventh line (byte 6 * 20).
DEFTYPE .l

!TestCase{splits}
  Dim c.l(5)
  For i=0 To 5 : c(i)=i*40 : Next
  m=AllocMem_(1024,$10000)
  !Require{m<>0,"memory"}
  CopperResetAdr m,0
  DoColSplit &c(0),2,1
  w0=Peek.l(m)
  !CheckEq{w0,$0001FFEE,"line0_word"}
  CopperResetAdr m+512,250
  DoColSplit &c(0),10,1
  w1=Peek.l(m+512+120)
  !CheckEq{w1,$FFE1FFFE,"line256_wait"}
  FreeMem_ m,1024
!EndTestCase

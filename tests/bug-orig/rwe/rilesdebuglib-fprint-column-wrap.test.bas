; rilesdebuglib-fprint-column-wrap (#707) - FPrint tests the column with
; CMPI.b #80 and BPL: a byte compare on the sign of the difference, so
; columns 208-255 print, at the cursor's row offset plus the column, into
; the rows below.  The bitmap is 640 x 64 (80 bytes per row, 8 text rows).
; Correct: a column of 80 or more prints nothing: the plane stays empty.
DEFTYPE .l

!TestCase{column_210}
  BitMap 0,640,64,1
  FLocate 210,4
  FPrint "from column 210 on"
  p=Peek.l(Addr BitMap(0)+8)
  k=0
  For i=0 To 5119 : k+Peek.b(p+i) AND 255 : Next
  !CheckEq{k,0,"plane_empty"}
!EndTestCase

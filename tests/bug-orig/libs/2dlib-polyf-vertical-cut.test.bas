; 2dlib-polyf-vertical-cut (#1033) - Polyf divides by zero when a side of
; the polygon starts beyond the clip edge it is cut at and its first edge is
; vertical: the cut at the side (roff/loff) divides by the x difference of
; the edge, which is 0.
; Here the points (320,200) (320,135) (-1,151) (-1,-1) (320,-1) on a 320x200
; bitmap, all in the word range (no clipping of #1000): they run
; anticlockwise, Polyf turns them round; the top line y = -1 spans the whole
; width (left start x = 320, right start x = -1), the start handling moves
; the left start in (entering from the right) and leaves the right one at
; x = -1, whose first edge is the vertical edge x = -1.  The original stops
; with "GURU! - Division by Zero".
; Correct: Polyf returns ("before", "after").
DEFTYPE .l

!TestCase{vertical_cut}
  BitMap 0,320,200,3
  Dim p.w(16)
  p(0)=320 : p(1)=200 : p(2)=320 : p(3)=135 : p(4)=-1 : p(5)=151
  p(6)=-1 : p(7)=-1 : p(8)=320 : p(9)=-1
  !Check{1,"before"}
  Polyf 5,&p(0),1,2
  !Check{1,"after"}
!EndTestCase

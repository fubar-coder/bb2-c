; polylib-vertical-cut (#1033) - BitMapPoly divides by zero when a side of
; the polygon starts beyond the clip edge it is cut at and its first edge is
; vertical: the cut at the side (roff/loff) divides by the x difference of
; the edge, which is 0 (the polygon code of 2dlib's Polyf).
; Here the points (320,-1) (-1,-1) (-1,151) (320,135) (320,200) on a 320x200
; bitmap of 1 plane (clockwise, as BitMapPoly wants them; 2dlib's test has
; them the other way round): the top line y = -1 spans the whole width
; (left start x = 320, right start x = -1), the start handling moves the
; left start in (entering from the right) and leaves the right one at
; x = -1, whose first edge is the vertical edge x = -1.  The original stops
; with "GURU! - Division by Zero".
; Correct: BitMapPoly returns ("before", "after").
DEFTYPE .l

!TestCase{vertical_cut}
  BitMap 0,320,200,1
  Dim p.w(16)
  p(0)=320 : p(1)=-1 : p(2)=-1 : p(3)=-1 : p(4)=-1 : p(5)=151
  p(6)=320 : p(7)=135 : p(8)=320 : p(9)=200
  Use BitMap 0
  !Check{1,"before"}
  VWait
  BLITZ
  BitMapPoly 5,&p(0),1
  VWait 2
  AMIGA
  !Check{1,"after"}
!EndTestCase

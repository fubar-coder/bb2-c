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
; @fixed #1033
DEFTYPE .l
BitMap 0,320,200,1
Dim p.w(16)
Restore pts
For i=0 To 9 : Read v : p(i)=v : Next
NPrint "before"
Use BitMap 0
VWait
BLITZ
BitMapPoly 5,&p(0),1
VWait 2
AMIGA
NPrint "after"
End
pts:
Data.l 320,-1,-1,-1,-1,151,320,135,320,200

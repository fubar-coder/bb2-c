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
; @fixed #1033
DEFTYPE .l
BitMap 0,320,200,3
Dim p.w(16)
Restore pts
For i=0 To 9 : Read v : p(i)=v : Next
NPrint "before"
Polyf 5,&p(0),1,2
NPrint "after"
End
pts:
Data.l 320,200,320,135,-1,151,-1,-1,320,-1

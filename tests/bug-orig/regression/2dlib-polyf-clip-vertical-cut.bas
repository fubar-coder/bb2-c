; 2dlib-polyf-clip-vertical-cut (#1034) - a port regression, not a bug of
; the original: the clipping of #1000 (clippoly) moves the points of a
; polygon outside the word range onto the lines x = -1, x = width, y = -1
; and y = height.  The clipped polygon can have a vertical edge on such a
; line at the start of a side, which reached the division by zero of the cut
; at the side (#1033): the original draws this polygon wrongly but returns,
; the port before the fix stopped with "GURU! - Division by Zero".
; Here t158_2dfuzz's self-intersecting polygon of 7 points (loop "polyf
; wild", k = 96 of the exact products of #199) on a 320x200 bitmap; clippoly
; turns it into (320,200) (320,200) (320,-1) (-1,-1) (-1,151) (320,135), whose
; right side starts on the vertical edge x = -1.
; Expected (what the original does): Polyf returns ("before", "after").
DEFTYPE .l
BitMap 0,320,200,3
Dim p.w(16)
Restore pts
For i=0 To 13 : Read v : p(i)=v : Next
NPrint "before"
Polyf 7,&p(0),1,2
NPrint "after"
End
pts:
Data.l 19371,-833,-4669,388,-19881,14092,-3847,-19682,4167,-18785,-1452,9618,8817,-4491

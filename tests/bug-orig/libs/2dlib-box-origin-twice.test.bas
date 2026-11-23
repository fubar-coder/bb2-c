; 2dlib-box-origin-twice (#391) - Box adds the bitmap origin
; (BitMapOrigin) to its corners and then draws the four sides with the
; line clipper, which adds the origin a second time: the outline is
; drawn displaced by twice the origin.
; Here BitMapOrigin 0,50,50 and Box 0,0,10,10 on a 320x200 bitmap; plane
; 0 is read at the pixels 50,50 (the corner with the origin once) and
; 100,100 (twice).
; Correct: the corner is at 50,50, as Line and Boxf put it; nothing at
; 100,100.
DEFTYPE .l

!TestCase{box_origin}
  BitMap 0,320,200,1
  BitMapOrigin 0,50,50
  Box 0,0,10,10,1
  p=Peek.l(Addr BitMap(0)+8)
  !Check{(Peek.b(p+50*40+6) AND $20)<>0,"corner_50_50"}
  !Check{(Peek.b(p+100*40+12) AND $8)=0,"nothing_at_100_100"}
!EndTestCase

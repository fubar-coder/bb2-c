; vallib-qangle-word-range (#1003) - QAngle subtracts the coordinates as
; words and takes the sign and the absolute value (NEG) of the word: a
; difference of 32768 or more in magnitude wraps (the word $8000 is
; -32768 and +32768 at once, NEG leaves it negative), so the angle is that
; of another quadrant or half plane.
; Here QAngle for differences of 32767, 32768 and 65535 along the axes
; and the diagonal, as the 32 bits of the Quick angle.
; Correct: the angle of the same direction with small differences, e.g.
; QAngle(0,0,-32768,0) = QAngle(0,0,-10,0) = $8001.
DEFTYPE .l

Function.l pa{x.w,y.w,u.w,v.w}
  a.q=QAngle(x,y,u,v)
  Function Return Peek.l(&a)
End Function

!TestCase{word_range}
  r=pa{0,0,32767,0}
  !CheckEq{r,$00000000,"right_max"}
  r=pa{0,0,-32768,0}
  !CheckEq{r,$00008001,"left_min"}
  r=pa{-32768,0,0,0}
  !CheckEq{r,$00000000,"from_left_min"}
  r=pa{0,0,0,-32768}
  !CheckEq{r,$0000C000,"up_min"}
  r=pa{0,-32768,0,0}
  !CheckEq{r,$00004001,"down_from_min"}
  r=pa{0,0,-32768,-32768}
  !CheckEq{r,$0000A000,"diagonal_min"}
  r=pa{32767,0,-32768,0}
  !CheckEq{r,$00008001,"wide_x"}
  r=pa{-32768,0,32767,0}
  !CheckEq{r,$00000000,"wide_x_back"}
  r=pa{0,32767,0,-32768}
  !CheckEq{r,$0000C000,"wide_y"}
  r=pa{32767,32767,-32768,-32768}
  !CheckEq{r,$0000A000,"wide_diagonal"}
!EndTestCase

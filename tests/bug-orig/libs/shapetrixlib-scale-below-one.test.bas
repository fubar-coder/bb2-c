; shapetrixlib-scale-below-one (#273) - Scale collects for every
; destination pixel the source pixels of the step 1/Ratio without a limit
; to the shape.  The step and the target size are both truncated, so
; normally the pixels stay inside; but a target size below 1 is raised to
; 1, and then the one destination pixel collects 1/Ratio source pixels
; from pixel 0 on: more than the shape has (the rest of the row, the next
; rows, behind the shape).
; Here: a 16x4 shape of one plane, row 0 colour 0, rows 1-3 colour 1;
; Scale with X Ratio 0.02 (target width 0 raised to 1, step 50) and
; Y Ratio 1.  Destination row 0 collects 16 pixels of row 0 and 34 of
; rows 1-3: the original gives colour 1, the most frequent.
; Correct: the one pixel of row 0 collects only the 16 pixels of the
; row: colour 0.  (Only row 0 is looked at: in the original row 3 reads
; behind the shape.)
DEFTYPE .l

!TestCase{scale_ratio_002}
  BitMap 0,16,4,1
  Use BitMap 0
  Cls 0
  Boxf 0,1,15,3,1
  GetaShape 0,0,0,16,4
  Scale 0,0.02,1
  d=Peek.l(Addr Shape(0)+14)
  !CheckEq{ShapeWidth(0),1,"width"}
  !CheckEq{ShapeHeight(0),4,"height"}
  !CheckEq{(Peek.b(d) AND $80) LSR 7,0,"colour"}
!EndTestCase

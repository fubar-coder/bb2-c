; shapetrixlib-rotate-rounding (#1004) - Rotate rounds the rotated
; coordinates (16.16 fixed point) with +$8000 for a positive value and
; -$8000 for a negative one, then takes the upper word (the floor): a
; negative value becomes floor(v - 0.5), one further from zero than the
; nearest integer.  The bounding box of the rotated corners gets an empty
; column or row on the negative side.
; Here: a filled 21x9 shape (one plane) is rotated by a quarter and by a
; half turn; printed are the new width and height and the number of set
; pixels of the shape's data.
; Correct: a quarter turn gives 9x21, a half turn 21x9, and all 189
; pixels are set in both.
; @fixed #1004
DEFTYPE .l
Function.l bits{s.l}
  a.l=Addr Shape(s) : d.l=Peek.l(a+14) : n.l=0
  For i.l=0 To (Peek.w(a+22) AND $FFFF)-1
    v.l=Peek.b(d+i) AND 255
    While v
      n=n+(v AND 1) : v=v LSR 1
    Wend
  Next
  Function Return n
End Function
BitMap 0,64,32,1
Use BitMap 0
Boxf 0,0,20,8,1
GetaShape 0,0,0,21,9
Rotate 0,0.25
NPrint "quarter ",ShapeWidth(0)," ",ShapeHeight(0)," pixels ",bits{0}
GetaShape 0,0,0,21,9
Rotate 0,0.5
NPrint "half ",ShapeWidth(0)," ",ShapeHeight(0)," pixels ",bits{0}
End

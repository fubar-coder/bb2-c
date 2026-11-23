; riintuilib-output-at-0-0 (#697) - the IPOutput handler prints with
; PrintIText at 0,0 of the Rast: the position of IPLocate (a Move on the
; Rast) has no effect, and the position does not advance, so each Print
; draws over the previous one at the top left corner.
; IPLocate 100,10, then Print "AB" and Print "C" (topaz 8: 8 x 8
; pixels a character).  The program counts the pixels set in three
; boxes: "AB" with its top left corner at 100,10 (x 100-115, y 10-17),
; "C" right after it (x 116-123, y 10-17), and the corner (x 0-23,
; y 0-9).
; Correct: "AB" at 100,10, "C" after it, nothing at the corner.
DEFTYPE .l

!TestCase{output_position}
  BitMap 0,320,32,2
  Cls 0
  IPBitMap 0,0
  Use Rast 0
  IPOutput 0
  IPLocate 100,10
  Print "AB"
  Print "C"
  DefaultOutput
  a=0 : c=0 : z=0
  For y=0 To 17
    For x=0 To 123
      If Point(x,y)
        If x>=100 AND x<=115 AND y>=10 Then a=a+1
        If x>=116 AND y>=10 Then c=c+1
        If x<=23 AND y<=9 Then z=z+1
      EndIf
    Next
  Next
  !CheckEq{a>0,-1,"ab"}
  !CheckEq{c>0,-1,"c"}
  !CheckEq{z>0,0,"corner"}
!EndTestCase

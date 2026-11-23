; bb-blitlib-createshape-past-max (#1077) - CreateShape fills count shape
; objects from start_shape on (32 bytes each) and never compares
; start_shape + count with the size of the shape table (Maximum Shapes,
; 100 by default).  CreateShape 99,2 writes the second object behind the
; table, over the 32 bytes at Addr Shape(99) + 32: its "cL" word ($634C)
; lands at +62.  The original has no such check, not even with runtime
; error checking (the compiler checks start_shape only).  The program
; saves the 32 bytes before and puts them back after.
; Here with runtime error checking.
; Correct: the error check refuses the count: "CreateShape start+count
; above Maximum Shapes!", nothing is written.
DEFTYPE .l

!TestCase{past_max}
  Dim s(7)
  BitMap 0,320,64,1
  Boxf 0,0,15,15,1
  GetaShape 0,0,0,16,16
  m=AllocMem_(256,$10002)
  !Require{m<>0,"memory"}
  a=Addr Shape(99)+32
  For i=0 To 7 : s(i)=Peek.l(a+i*4) : Next
  !ExpectError{"CreateShape start+count above Maximum Shapes!","refused"}
  CreateShape 99,2,m,0
  w=Peek.w(a+30)
  For i=0 To 7 : Poke.l a+i*4,s(i) : Next
  !CheckEq{w,0,"behind_table"}
  FreeMem_ m,256
!EndTestCase

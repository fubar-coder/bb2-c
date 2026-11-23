; compiler-newtype-forward-mixed (#974) - .ta contains the forward type .tb
; by value, .tb has a pointer to .ta back: no cycle, both compile.
; Correct: .tb has 8 bytes (back at 0, v at 4), .ta 12 (n at 0, x at 2,
; m at 10); the pointer leads back to the variable.
NEWTYPE .pp
  *p.tb
End NEWTYPE
NEWTYPE .ta
  n.w
  x.tb
  m.w
End NEWTYPE
NEWTYPE .tb
  *back.ta
  v.l
End NEWTYPE
DEFTYPE .ta s

!TestCase{forward_mixed}
  s\n=1
  s\x\back=&s
  s\x\v=5
  s\m=3
  !CheckEq{SizeOf .ta,12,"size_ta"}
  !CheckEq{SizeOf .ta\x,2,"offset_x"}
  !CheckEq{SizeOf .ta\m,10,"offset_m"}
  !CheckEq{SizeOf .tb,8,"size_tb"}
  !CheckEq{SizeOf .tb\v,4,"offset_v"}
  !CheckEq{s\n,1,"n"}
  !CheckEq{s\x\v,5,"x_v"}
  !CheckEq{s\m,3,"m"}
  !CheckEq{s\x\back\m,3,"back_m"}
!EndTestCase

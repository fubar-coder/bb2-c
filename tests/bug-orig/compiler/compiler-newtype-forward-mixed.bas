; compiler-newtype-forward-mixed (#974) - .ta contains the forward type .tb
; by value, .tb has a pointer to .ta back: no cycle, both compile.
; Correct: .tb has 8 bytes (back at 0, v at 4), .ta 12 (n at 0, x at 2,
; m at 10); the pointer leads back to the variable.
; @fixed #974
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
s\n=1
s\x\back=&s
s\x\v=5
s\m=3
NPrint "a ",SizeOf .ta," x ",SizeOf .ta\x," m ",SizeOf .ta\m
NPrint "b ",SizeOf .tb," v ",SizeOf .tb\v
NPrint s\n," ",s\x\v," ",s\m," ",s\x\back\m
End

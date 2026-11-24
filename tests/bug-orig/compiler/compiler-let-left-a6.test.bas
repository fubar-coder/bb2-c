; compiler-let-left-a6 - the left side of an assignment whose address has
; code (an array index) is compiled before the right side, and its code
; is moved behind the right side's (dolet2's pushstart, pushdo, pushput,
; blitz2.s:14340, :11022-11057).  The compiler notes the base a6 holds
; (lasta6) as the code is compiled, not as it runs: a library function in
; the index whose base is passed in a6 (Abs of a Word: ffplib's base)
; loads a6 in the left side's code, and the right side's call of a
; function with the same base skips its load of a6 - but the right
; side's code runs first, with whatever a6 holds then.  Here a6 holds
; mathieeesingbas.library's base from the statement before (IEEESPFix_):
; the right side's Abs and its conversion go through the IEEE library and
; the value stored is wrong; with another base in a6 the program crashes.
; Correct: a(3) is 5, and the right side loads a6 itself.
DEFTYPE .l
!TestCase{left_a6}
  Dim a.l(4)
  j.w = 3 : i = $3F800000
  k.l = IEEESPFix_(i)
  a(Abs(-j)) = Abs(-5)
  !CheckEq{a(3),5,"right_side"}
!EndTestCase

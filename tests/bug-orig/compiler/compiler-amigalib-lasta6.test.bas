; compiler-amigalib-lasta6 - an OS library function in an expression
; (amigalib, blitz2.s:10183) loads its library's base into a6 (move.l
; off(a5),a6) but leaves lasta6, the compiler's note of the base a6
; holds, as it was.  After a Float operation (or a library function that
; takes a base in a6) earlier in the same expression, the compiler takes
; a6 for that base still: the FFP calls after the OS call (the conversion
; of its result to a Float, the addition) go through the OS library's
; base.  With mathieeesingbas.library, whose vectors are laid out as
; mathffp.library's, the expression computes a wrong value; with any
; other library (exec's FindTask_) the program crashes (a Privilege
; Violation).  An OS call before the Float operation is not affected.
; Correct: 1.5*2+IEEESPFix_(1.0) is 4 in both orders.
DEFTYPE .l
!TestCase{float}
  x.f=1.5 : y.f=2 : i=$3F800000
  f.f=x*y+IEEESPFix_(i)
  !CheckFltNear{f,4,0.0001,"after_a_float_operation"}
  f=IEEESPFix_(i)+x*y
  !CheckFltNear{f,4,0.0001,"before_a_float_operation"}
!EndTestCase

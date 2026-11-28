; expr-tsr-div0-checked (#1235, type-system-rework) - a division or Mod by
; a run-time 0 with the runtime error checking on (spec compiler/
; expression-types, "A division or Mod by a run-time 0 raises the division
; trap": "with the runtime error checking on, the checks' Division by
; Zero error stays where it is given today"): every type, every
; destination, a quotient that keeps its fraction and Mod end in the
; error "Division by Zero"; the check behind it does not run.  A constant
; divisor 0 stays the compile error (expr-tsr-div0-const).
; The port already stops in every case here; the original runs on in the
; cases it does not stop at without the checking (expr-tsr-div0-trap),
; which its cells declare per case.
; Correct: each case raises the error at its division.

; a Byte divisor
!TestCase{byte}
  vb.b=7 : zb.b=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vb/zb
  !Check{0,"after"}
!EndTestCase

; a Word divisor
!TestCase{word}
  vw.w=7 : zw.w=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vw/zw
  !Check{0,"after"}
!EndTestCase

; a Long divisor
!TestCase{long}
  vl.l=100 : zl.l=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vl/zl
  !Check{0,"after"}
!EndTestCase

; a Quick divisor
!TestCase{quick}
  vl.l=100 : zq=0
  !ExpectError{"Division by Zero","trap"}
  rq.q=vl/zq
  !Check{0,"after"}
!EndTestCase

; a Float divisor
!TestCase{float}
  vf.f=1 : zf.f=0
  !ExpectError{"Division by Zero","trap"}
  rf.f=vf/zf
  !Check{0,"after"}
!EndTestCase

; a 48.16 divisor (the product of two Quicks)
!TestCase{wide}
  vl.l=100 : qa=0 : qb=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vl/(qa*qb)
  !Check{0,"after"}
!EndTestCase

; a whole quotient that keeps its fraction under a Quick
!TestCase{whole_quotient_quick}
  vw.w=7 : zw.w=0
  !ExpectError{"Division by Zero","trap"}
  rq.q=vw/zw
  !Check{0,"after"}
!EndTestCase

; a whole quotient that keeps its fraction under a Float
!TestCase{whole_quotient_float}
  vw.w=7 : zw.w=0
  !ExpectError{"Division by Zero","trap"}
  rf.f=vw/zw
  !Check{0,"after"}
!EndTestCase

; the inner quotient of (a/b)/c divides by 0
!TestCase{nested_left}
  vw.w=7 : zw.w=0 : cw.w=2
  !ExpectError{"Division by Zero","trap"}
  rq.q=(vw/zw)/cw
  !Check{0,"after"}
!EndTestCase

; the divisor quotient of a/(d/b) is 0
!TestCase{nested_right}
  vw.w=7 : zw.w=0 : bw.w=2
  !ExpectError{"Division by Zero","trap"}
  rq.q=vw/(zw/bw)
  !Check{0,"after"}
!EndTestCase

; Mod by a Byte 0
!TestCase{mod_byte}
  vb.b=7 : zb.b=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vb Mod zb
  !Check{0,"after"}
!EndTestCase

; Mod by a Word 0
!TestCase{mod_word}
  vw.w=7 : zw.w=0
  !ExpectError{"Division by Zero","trap"}
  rw.w=vw Mod zw
  !Check{0,"after"}
!EndTestCase

; Mod by a Long 0
!TestCase{mod_long}
  vl.l=-7 : zl.l=0
  !ExpectError{"Division by Zero","trap"}
  rl.l=vl Mod zl
  !Check{0,"after"}
!EndTestCase

; Mod by a Quick 0
!TestCase{mod_quick}
  vq=7.5 : zq=0
  !ExpectError{"Division by Zero","trap"}
  rq.q=vq Mod zq
  !Check{0,"after"}
!EndTestCase

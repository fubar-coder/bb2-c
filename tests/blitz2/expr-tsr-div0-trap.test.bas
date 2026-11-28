; expr-tsr-div0-trap (#1235, type-system-rework) - a division or Mod by a
; run-time 0 raises the CPU's division trap for every type (spec compiler/
; expression-types, "A division or Mod by a run-time 0 raises the division
; trap"): Byte, Word, Long, Quick, 48.16 and Float divisors, under every
; destination, also a quotient whose fraction an exception keeps (a whole
; quotient under a Quick or a Float, the nested forms of the answer I4)
; and Mod.  Without runtime error checking the trap is not caught: the
; program stops in the system's "Software Failure" requester (the test
; configuration asserts it for each case: the requester, no return code,
; the case and the run do not end); expr-tsr-div0-checked is the same
; with the checking on, where the debugger reports "Division by Zero".
; The original stops at a Float divisor, at a whole quotient under a Float
; and at a Word or Quick Mod, and goes on with a value in the other cases
; (a Byte, Word, Long, Quick and 48.16 divisor, a whole quotient under a
; Quick, the nested forms, a Byte and Long Mod); its cells declare each
; case that goes on.
; Correct: each case stops at its division.

; a Byte divisor
!TestCase{byte}
  vb.b=7 : zb.b=0
  rl.l=vb/zb
  !Check{0,"after"}
!EndTestCase

; a Word divisor
!TestCase{word}
  vw.w=7 : zw.w=0
  rl.l=vw/zw
  !Check{0,"after"}
!EndTestCase

; a Long divisor
!TestCase{long}
  vl.l=100 : zl.l=0
  rl.l=vl/zl
  !Check{0,"after"}
!EndTestCase

; a Quick divisor
!TestCase{quick}
  vl.l=100 : zq=0
  rq.q=vl/zq
  !Check{0,"after"}
!EndTestCase

; a Float divisor
!TestCase{float}
  vf.f=1 : zf.f=0
  rf.f=vf/zf
  !Check{0,"after"}
!EndTestCase

; a 48.16 divisor (the product of two Quicks)
!TestCase{wide}
  vl.l=100 : qa=0 : qb=0
  rl.l=vl/(qa*qb)
  !Check{0,"after"}
!EndTestCase

; a whole quotient that keeps its fraction under a Quick
!TestCase{whole_quotient_quick}
  vw.w=7 : zw.w=0
  rq.q=vw/zw
  !Check{0,"after"}
!EndTestCase

; a whole quotient that keeps its fraction under a Float
!TestCase{whole_quotient_float}
  vw.w=7 : zw.w=0
  rf.f=vw/zw
  !Check{0,"after"}
!EndTestCase

; the inner quotient of (a/b)/c divides by 0
!TestCase{nested_left}
  vw.w=7 : zw.w=0 : cw.w=2
  rq.q=(vw/zw)/cw
  !Check{0,"after"}
!EndTestCase

; the divisor quotient of a/(d/b) is 0
!TestCase{nested_right}
  vw.w=7 : zw.w=0 : bw.w=2
  rq.q=vw/(zw/bw)
  !Check{0,"after"}
!EndTestCase

; Mod by a Byte 0
!TestCase{mod_byte}
  vb.b=7 : zb.b=0
  rl.l=vb Mod zb
  !Check{0,"after"}
!EndTestCase

; Mod by a Word 0
!TestCase{mod_word}
  vw.w=7 : zw.w=0
  rw.w=vw Mod zw
  !Check{0,"after"}
!EndTestCase

; Mod by a Long 0
!TestCase{mod_long}
  vl.l=-7 : zl.l=0
  rl.l=vl Mod zl
  !Check{0,"after"}
!EndTestCase

; Mod by a Quick 0
!TestCase{mod_quick}
  vq=7.5 : zq=0
  rq.q=vq Mod zq
  !Check{0,"after"}
!EndTestCase

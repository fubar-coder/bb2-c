; frac-string - Frac takes any numeric type since #1064 (#unknown, as Int
; and Sgn), so the compiler no longer refuses a string; the error check
; intchk refuses it at runtime, as for Int (frac-string.test.toml sets the
; runtime error checking). Frac of a Float, then Frac of a string.
; Correct: it compiles, Frac(1.25) is .25, Frac(a$) is the runtime error
; "Illegal Type". (The original refuses Frac(a$) at compile time with "Can't
; Convert Types", which is correct there: Frac takes only a Float.)
!TestCase{frac_string}
  f.f=1.25
  !CheckFltNear{Frac(f),0.25,0.001,"float"}
  a$="1.5"
  !ExpectError{"Illegal Type"}
  NPrint "string ",Frac(a$)
!EndTestCase

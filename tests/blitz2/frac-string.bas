; @runerrs on
; Frac takes any numeric type since #1064 (#unknown, as Int and Sgn), so
; the compiler no longer refuses a string; the error check intchk refuses
; it at runtime, as for Int.  With runtime error checking: Frac of a
; Float, then Frac of a string.
; Expected: it compiles, Frac(1.25) is .25, Frac(a$) is the runtime error
; "Illegal Type".  (The original refuses Frac(a$) at compile time with
; "Can't Convert Types", which is correct there: Frac takes only a Float.)
f.f=1.25 : NPrint "float ",Frac(f)
a$="1.5"
NPrint "string ",Frac(a$)
End

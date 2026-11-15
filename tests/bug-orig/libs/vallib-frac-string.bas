; vallib-frac-string (#1064) - Frac took a Float (!args #float), so the
; compiler refused a string argument; since #1064 Frac takes any numeric
; type (#unknown, as Int and Sgn), and a string is refused by the error
; check intchk at runtime, as for Int.  Here with runtime error checking:
; Frac of a Float, then Frac of a string.
; Correct: it compiles, Frac(1.25) is .25, Frac(a$) is the runtime error
; "Illegal Type" (the original: "Can't Convert Types" at Frac(a$)).
; @fixed #1064
; @configs clib
; @pass runtime_errors=on
f.f=1.25 : NPrint "float ",Frac(f)
a$="1.5"
NPrint "string ",Frac(a$)
End

; printlib-quick-trailing-point (#234) - Print of a Quick removes the
; trailing zeros of its four decimals with a loop ('f0l) that compares the
; point with the same character it has just found to be "0", so it never
; stops at the point: a fraction that rounds to .0000 leaves a lone ".".
; Here Print and Str$ of 1+1/65536, 1/65536, -1/65536 and 5+1/65536.
; Correct: the point goes with the zeros: "1", "0", "0" (no sign for a
; value that rounds to 0) and "5".
DEFTYPE .l

!TestCase{lone_point}
  q.q=0
  Poke.l &q,$00010001 : a$=Str$(q) : NPrint "quick 00010001 <",q,"> <",a$,">"
  Poke.l &q,$00000001 : b$=Str$(q) : NPrint "quick 00000001 <",q,"> <",b$,">"
  Poke.l &q,$FFFFFFFF : c$=Str$(q) : NPrint "quick FFFFFFFF <",q,"> <",c$,">"
  Poke.l &q,$00050001 : d$=Str$(q) : NPrint "quick 00050001 <",q,"> <",d$,">"
  !CheckStrEq{a$,"1","one"}
  !CheckStrEq{b$,"0","tiny"}
  !CheckStrEq{c$,"0","tiny_negative"}
  !CheckStrEq{d$,"5","five"}
!EndTestCase

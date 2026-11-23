; printlib-quick-fraction-carry (#233) - Print of a Quick rounds the
; fraction to four decimals (fraction * 10000, rounded); a fraction that
; rounds to 10000 is printed as the four digits "0000" and the carry into
; the integer part is lost: 2.99999 is printed as "2." (with #234 fixed:
; "2").
; Here Print and Str$ of $0002FFFF (2.99998), $0000FFFF (0.99998),
; $7FFFFFFF (32767.99998) and -$0002FFFF.
; Correct: the integer part grows by one: "3", "1", "32768", "-3".
DEFTYPE .l

!TestCase{fraction_carry}
  q.q=0
  Poke.l &q,$0002FFFF : a$=Str$(q) : NPrint "quick 0002FFFF <",q,"> <",a$,">"
  Poke.l &q,$0000FFFF : b$=Str$(q) : NPrint "quick 0000FFFF <",q,"> <",b$,">"
  Poke.l &q,$7FFFFFFF : c$=Str$(q) : NPrint "quick 7FFFFFFF <",q,"> <",c$,">"
  Poke.l &q,-$0002FFFF : d$=Str$(q) : NPrint "quick FFFD0001 <",q,"> <",d$,">"
  !CheckStrEq{a$,"3","x2_99998"}
  !CheckStrEq{b$,"1","x0_99998"}
  !CheckStrEq{c$,"32768","max"}
  !CheckStrEq{d$,"-3","negative"}
!EndTestCase

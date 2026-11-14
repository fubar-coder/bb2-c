; printlib-quick-fraction-carry (#233) - Print of a Quick rounds the
; fraction to four decimals (fraction * 10000, rounded); a fraction that
; rounds to 10000 is printed as the four digits "0000" and the carry into
; the integer part is lost: 2.99999 is printed as "2." (with #234 fixed:
; "2").
; Here Print and Str$ of $0002FFFF (2.99998), $0000FFFF (0.99998),
; $7FFFFFFF (32767.99998) and -$0002FFFF.
; Correct: the integer part grows by one: "3", "1", "32768", "-3".
; @fixed #233
; @configs clib
Statement pq{x.l}
  q.q=0 : Poke.l &q,x
  NPrint "quick ",Hex$(x)," <",q,"> <",Str$(q),">"
End Statement
pq{$0002FFFF} : pq{$0000FFFF} : pq{$7FFFFFFF} : pq{-$0002FFFF}
End

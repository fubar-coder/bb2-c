; printlib-quick-trailing-point (#234) - Print of a Quick removes the
; trailing zeros of its four decimals with a loop ('f0l) that compares the
; point with the same character it has just found to be "0", so it never
; stops at the point: a fraction that rounds to .0000 leaves a lone ".".
; Here Print and Str$ of 1+1/65536, 1/65536, -1/65536 and 5+1/65536.
; Correct: the point goes with the zeros: "1", "0", "0" (no sign for a
; value that rounds to 0) and "5".
; @fixed #234
; @configs clib
Statement pq{x.l}
  q.q=0 : Poke.l &q,x
  NPrint "quick ",Hex$(x)," <",q,"> <",Str$(q),">"
End Statement
pq{$00010001} : pq{$00000001} : pq{$FFFFFFFF} : pq{$00050001}
End

; ldivlib-div0 (#200) - the Long and Quick division of the 68000 (ldivlib)
; tests the divisor for 0 before it looks at the signs and returns
; $7FFFFFFF, the largest positive number, for any dividend: -5/0 is
; positive.
; Correct: the result has the sign of the dividend: -$7FFFFFFF for a
; negative dividend, $7FFFFFFF for a positive one and for 0.
; @fixed #200
; @configs clib
a.l=-5 : b.l=0
NPrint "a ",a/b
a=5 : NPrint "b ",a/b
a=0 : NPrint "c ",a/b
q.q=-5 : r.q=0 : s.q=q/r : NPrint "d ",Hex$(Peek.l(&s))
q=5 : s=q/r : NPrint "e ",Hex$(Peek.l(&s))
End

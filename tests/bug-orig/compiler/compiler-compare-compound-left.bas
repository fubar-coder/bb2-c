; compiler-compare-compound-left (#1183) - the type an expression is
; wanted in reaches the operands of a comparison.  The compiler emits
; the code of the left operand before it reads the comparison operator,
; so when the left operand is an expression (q*2, q+q) it is computed in
; that type: with q=1.25 and a Long destination q*2>2.4 computes 2>2.4
; (0; the original cut q to 1 first, 1*2>2.4).
; Correct: the operands are compared with their own values: q*2 is 2.5
; and q+q is 2.5, so all three comparisons are true (-1).
; Needs the expression tree (#1180); the left operand that is a single
; element is #1177 (compiler-quick-compare).
q.q=1.25 : r.l=0
r=(q*2>2.4) : NPrint "01 r.l=(q*2>2.4) ",r
r=(q+q>2.4) : NPrint "02 r.l=(q+q>2.4) ",r
r=(q*2=2.5) : NPrint "03 r.l=(q*2=2.5) ",r
End

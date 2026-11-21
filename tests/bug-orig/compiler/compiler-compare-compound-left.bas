; compiler-compare-compound-left (#1183) - the type an expression is
; wanted in reaches the operands of a comparison, and a Quick fetched
; while that type is Long is cut to its whole part.  The compiler emits
; the code of the left operand before it reads the comparison operator,
; so when the left operand is an expression (q*2, q+q) the cut happens
; inside it: with q=1.5 and a Long destination q*2>3 computes 1*2>3 (0).
; Correct: the operands are compared with their own values: q*2 is 3.0
; and q+q is 3.0, so all three comparisons are true (-1).
; Needs the expression tree (#1180); the left operand that is a single
; element is #1177 (compiler-quick-compare).
q.q=1.5 : r.l=0
r=(q*2>2.5) : NPrint "01 r.l=(q*2>2.5) ",r
r=(q+q>2) : NPrint "02 r.l=(q+q>2) ",r
r=(q*2=3) : NPrint "03 r.l=(q*2=3) ",r
End

; compiler-operand-beyond-d7 (#1185) - an expression gets a register per nesting
; level, d0 to d7; getelement (blitz2.s:9643) handles the element at the
; 8th register (regat 7: it is computed in d6 and moved to d7) but not one
; more: the right operand of an operator whose left operand is in d7 is
; fetched into d7 over the left operand, and the operator takes its right
; operand from "d8", whose register field is a0 (cmp.l a0,d7).  So
; k AND m>l inside six brackets of additions compares k with an address
; (false); k+m+l there is right (each addition is done before the next
; operand is fetched).
; Correct: the compile error "Expression too Complex" at the line of
; k AND m>l (the error of the expression stacks), no wrong code.  The
; other shapes: compiler-operand-beyond-d7-arg7 (a 7th argument),
; -word (Word operands), -const (constants).
; @fixed #1185
c.l=1 : d.l=2 : e.l=3 : h.l=4 : i.l=5 : j.l=6 : k.l=-1 : m.l=3 : l.l=1
r.l=c+(d+(e+(h+(i+(j+(k AND m>l)))))) : NPrint "01 k AND m>l ",r
r=c+(d+(e+(h+(i+(j+(k+m+l)))))) : NPrint "02 k+m+l ",r
End

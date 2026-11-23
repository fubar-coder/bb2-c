; compiler-compare-right-operand-refused (#1182) - a Float left operand
; of a bit operator whose right operand is a comparison.
; BUG(orig) (#1182): the comparison's boolean is a Word, and the left
; operand is converted down to Word before the operation: with fh=0.5
; rf.f=fh AND (l1=l1) computes 0 AND -1 and gives 0.
; Correct: the boolean is converted up to the left operand's type; a
; Float has no AND, OR, EOR or shift, so the expression is refused with
; "Illegal Operator for Type", as rf.f=fh AND w is (spec
; compiler/expression-types).
; @fixed-orig #1182
l1.l=1 : fh.f=0.5 : rf.f=0
rf=fh AND (l1=l1) : NPrint "01 rf.f=fh AND (l1=l1) ",rf
End

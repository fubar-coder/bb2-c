; compiler-operand-order-print (#1216) - the type and the format of an
; operation depend on the order of its operands and the place of a
; subexpression in a chain: the Print forms of compiler-operand-order.
; BUG(orig) (#1216): NPrint has no wanted type, so the type so far starts
; as the first operand's: 1+1+1+q is a whole number (4) and w+v+q too
; (8), q+w+v is the Quick sum (8.5); w/2 beside a Quick is the whole-
; number division before it and the fractional one after it; a Quick and
; an address in q+&l-&l are added and subtracted in 16.16 (1, the
; fraction lost).
; Correct (spec compiler/expression-types, "A Quick keeps its fraction
; when nothing asks for a whole number", "The type and format of an
; operation do not depend on the order of its operands"): the Quick keeps
; its fraction (4.5, 8.5 and 8.5, 13.5), w/2 beside a Quick is 1.5 in
; both orders (2), q+&l-&l is 1.5, the Word sum that overflows wraps the
; same way in both orders; the original's values are the declared
; deviations.  The decided rule is P+ (design.md of
; expr-tree-5-operand-typing, decisions 6 and 7).
!TestCase{prints}
  q.q=1.5 : w.w=3 : v.w=4 : l.l=10
  NPrint "01 1+1+1+q ",1+1+1+q
  NPrint "02 w+v+q ",w+v+q
  NPrint "03 q+w+v ",q+w+v
  NPrint "04 w*v+q ",w*v+q
  NPrint "05 q+&l-&l ",q+&l-&l
  NPrint "06 Int(q)*100000 ",Int(q)*100000
  q=0.5
  NPrint "07 q+w/2 ",q+w/2
  NPrint "08 w/2+q ",w/2+q
  w=20000 : v=20000
  NPrint "09 w+v+q ",w+v+q
  NPrint "10 q+w+v ",q+w+v
!EndTestCase

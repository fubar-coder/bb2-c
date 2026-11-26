; expr-pairs - the register pairs of 48.16 intermediate results (#1216,
; design decision 10): a 48.16 value takes two registers, the right
; operand of an operation on it the register after them; one that would
; need a register beyond d7 is "Expression too Complex", as #1185 (the
; data sources of expr-pairs.test/, asserted in expr-pairs.test.toml:
; checked when the expression is built, so an error read later in the same
; expression comes first, an error of a later statement after it).
; BUG(orig) (#1216): the original computes them in the type so far (19
; and 17) and has no register pairs (it compiles the data sources).
; Correct: the case gives the values of the forms one level shallower.
!TestCase{pairs}
  q.q=1.5 : l.l=2 : c1.l=1 : c2.l=2 : e.l=3 : h.l=4 : i.l=5 : r.l=0
  r=c1+(c2+(e+(h+(i+(q*l*l)))))
  !CheckEq{r,21,"pair_d7"}
  r=c1+(c2+(e+(h+(i+(q*l)))))
  !CheckEq{r,18,"pair_d6"}
!EndTestCase

; join-op - joining lines with @@ after an operator (spec
; ab3-compat/syntax-rules, "Line joining with @@"): the expression goes on
; in the next line. AmiBlitz3 gives a syntax error there
; (join-op.test.toml)
!TestCase{join_op}
  !CheckStrEq{"start","start","l1"}
  b.l = 1 + @@
  2
  !CheckEq{b,3,"l2"}
!EndTestCase

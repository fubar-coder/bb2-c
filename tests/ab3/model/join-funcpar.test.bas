; join-funcpar - joining lines with @@ after a comma inside a parenthesised
; parameter list with a parameter still to come (spec
; ab3-compat/syntax-rules, "Line joining with @@"). AmiBlitz3: "a bracket
; ) is expected here" (join-funcpar.test.toml)
OPTIMIZE 4
!TestCase{join_funcpar}
  zs$ = Left$("abc", @@
  2)
  !CheckStrEq{zs$,"ab","l1"}
  r.l = Instr("abc", @@
  "b")
  !CheckEq{r,2,"l2"}
  r = QLimit(5, @@
  1,3)
  !CheckEq{r,3,"l3"}
!EndTestCase

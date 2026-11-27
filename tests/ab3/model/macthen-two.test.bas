; macthen-two - two one-line Ifs on one macro line: one End If each (spec
; ab3-compat/syntax-rules; AmiBlitz3 appends one, the outer If stays open
; to the end of the expanded line: macthen-two.test.toml)
Macro t
If `1 > 0 Then If `2 > 0 Then `2 = 0
`3 = 1
End Macro
Macro u
If `1 > 0 Then If `2 > 0 Then `2 = 0 : End If
`3 = 1
End Macro
!TestCase{macthen_two}
  a.l = 0 : b.l = 5 : c.l = 0 : !t{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"0 5 1","l1"}
  a = 1 : b = 5 : c = 0 : !t{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"1 0 1","l2"}
  a = 0 : b = 5 : c = 0 : !u{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"0 5 1","l3"}
!EndTestCase

; macthen-endifword - EndIf closes a one-line If on a macro line like End
; If (spec ab3-compat/syntax-rules; AmiBlitz3 appends a second End If: "End
; condition without condition", macthen-endifword.test.toml)
Macro a
If `1 > 0 Then `1 = 0 : EndIf
`2 = `2 + 10
End Macro
!TestCase{macthen_endifword}
  x.l = 1 : y.l = 5 : !a{x, y}
  !CheckStrEq{Str$(x)+" "+Str$(y),"0 15","l1"}
!EndTestCase

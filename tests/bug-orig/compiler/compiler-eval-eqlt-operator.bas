; compiler-eval-eqlt-operator (#9) - eval2 reads "=" and then "<" and falls
; into .isne (blitz2.s:9133): "=<" is compiled as "<>" instead of
; less-or-equal. "<=", ">=", "=>", "<>" and "><" are right.
; Correct: "=<" is less-or-equal, as "<=".
; @fixed #9
a=1:b=2:NPrint a=<b
a=2:b=2:NPrint a=<b
a=3:b=2:NPrint a=<b
x.f=1.5:y.f=1.5:NPrint x=<y
NPrint 3=<2
For i=1 To 3
  If i=<2 Then NPrint "le ",i Else NPrint "gt ",i
Next
End

; BB2 mode: a one-line If in a macro stays open to the end of the expanded
; line, as with blitz2: not a bug (#967), the programmer writes the End If
Macro a
If `1 > 0 Then `1 = 0
End Macro
Macro b
While `1 > 0
  !a{`1}
Wend
End Macro
x.l = 1 : !b{x} : NPrint x
End

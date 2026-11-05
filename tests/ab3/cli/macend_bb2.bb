; BB2 mode: a macro line ending in End loses the rest of the macro, as
; with blitz2 (#968): "If Without End If" at the call
Macro m
If `1
End
End If
NPrint "after"
End Macro
!m{0}
NPrint "done"
End

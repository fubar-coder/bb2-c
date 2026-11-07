; BB2 mode: a macro line ending in End keeps the rest of the macro, as in
; AB3 mode (Blitz Basic 2 loses it, "If Without End If" at the call; fixed,
; #968)
Macro m
If `1
End
End If
NPrint "after"
End Macro
!m{0}
NPrint "done"
End

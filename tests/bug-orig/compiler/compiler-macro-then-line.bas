; compiler-macro-then-line (#967) - domacro stores a macro body as one line,
; every line end of the definition becomes a ':'.  A one-line If ... Then is
; closed only at the end of a physical line (doline), not at a ':'.  In a
; macro the rest of the body and the rest of the calling line belong to the
; condition: the Wend of macro b finds the one-liner of macro a on top of
; the condition stack ("Illegaly nested Condition"), and the line after the
; one-liner of macro c runs only when the condition holds.
; Correct (AmiBlitz3, which ends a macro line with Then with ": End If"):
; the program prints 0 and 15.
Macro a
If `1 > 0 Then `1 = 0
End Macro
Macro b
While `1 > 0
  !a{`1}
Wend
End Macro
x.l = 1 : !b{x} : NPrint x
Macro c
If `1 > 0 Then `1 = 0
`2 = `2 + 10
End Macro
y.l = 0 : z.l = 5 : !c{y, z} : NPrint z
End

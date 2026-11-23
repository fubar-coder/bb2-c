; expr-limits.test/brackets64.bas: 64 nested parentheses, one more than
; the precedence stack takes
Macro o
((((((((
End Macro
Macro c
))))))))
End Macro
b.l = 3
b = !o!o!o!o!o!o!o!o b!c!c!c!c!c!c!c!c
NPrint b
End

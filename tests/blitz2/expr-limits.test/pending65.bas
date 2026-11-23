; expr-limits.test/pending65.bas: the expression, + and *, a parenthesis,
; NOT and 60 parentheses, one entry more than the precedence stack takes
Macro o
((((((((
End Macro
Macro c
))))))))
End Macro
v1.l = 1 : v2.l = 2 : b.l = 3
r.l = v1 + (v2 * NOT !o!o!o!o!o!o!o((((b))))!c!c!c!c!c!c!c)
NPrint r
End

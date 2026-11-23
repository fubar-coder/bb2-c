; expr-limits.test/minus64.bas: 64 times a minus sign and a parenthesis,
; one parenthesis more than the precedence stack takes
Macro mb
-(-(-(-(-(-(-(-(
End Macro
Macro c
))))))))
End Macro
b.l = 3
r.l = !mb!mb!mb!mb!mb!mb!mb!mb b!c!c!c!c!c!c!c!c
NPrint r
End

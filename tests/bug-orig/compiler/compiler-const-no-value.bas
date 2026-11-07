; compiler-const-no-value (#972) - a constant defined without a value
; (a line "#c", no "=") is accepted, but handleconst never sets its value:
; the node comes from addhere, which in the original has no MEMF_CLEAR, so
; the constant holds what the memory held (inject fill=a5: $a5a5a5a5-derived,
; non-zero, so CNIF takes the true branch). "#d=5" followed by "#d" keeps 5.
; Correct (decision as AmiBlitz3 in practice): a constant without an
; explicitly assigned value is 0, CNIF is false, and the value does not
; change with the memory.
; @fixed #972
; @pass wrap=@INJECT@|fill=a5
#c
#d=5
#d
NPrint #c
CNIF #c
NPrint "cnif true"
CELSE
NPrint "cnif false"
CEND
NPrint #d
End

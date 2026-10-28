; t63_serialerr - seriallib: a unit that is not open gives the runtime
; error "Serial Device Not Open" (TRAP #0 in the normal code, also without
; the debugger; without it the original runs into the message text)
NPrint "before"
WriteSerial 0,65
NPrint "after"
End

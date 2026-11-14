; gadgetslib-stringgadget-maxlen0 (#165) - StringGadget allocates the
; buffer with Maxlen bytes and clears it with SUBQ #1 and a DBF loop: with
; Maxlen 0 (a string gadget without room even for the 0 at its end, a
; programming error) it clears the 65536 bytes behind its 0 byte buffer.
; The original has no check routine for StringGadget, so runtime error
; checking does not catch it either.  With runtime error checking.
; Correct: the runtime error "Maxlen MUST be greater than zero", and the
; gadget is not made.
; @fixed #165
; @configs clib
; @pass runtime_errors=on
NPrint "before"
StringGadget 0,10,20,0,1,0,100
NPrint "after"
End

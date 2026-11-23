; rundebuglib-ctrlc-no-runerrs (#43) - the Ctrl-C check that ends an
; executable compiled with runtime error checking at its next Print,
; NPrint, Edit$ or Edit on the console lies in the error check area: a
; program compiled without runtime error checking has none and is not
; changed.  The case sends itself Ctrl-C (exec SetSignal of
; SIGBREAKF_CTRL_C), prints to the console, clears the signal.
; Correct: as the original, the program runs to its end, return code 0.
; A regression test of the fix of #43.

!TestCase{ctrlc_without_error_check}
  SetSignal_ $1000,$1000
  NPrint "signalled"
  SetSignal_ 0,$1000
  !Check{-1,"not_stopped"}
!EndTestCase

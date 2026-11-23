; rundebuglib-ctrlc-console (#43) - an executable compiled with runtime
; error checking cannot be stopped with Ctrl-C: rundebuglib's TRAP #1
; handler, which every statement calls, tests a word ctrlc that nothing
; sets.  The program sends itself Ctrl-C (exec SetSignal of
; SIGBREAKF_CTRL_C) and prints to the console; then it clears the
; signal and goes on (the break is taken at the end of a Print).
; Correct (as a C program on the console): the statement after the signal
; ends the program with "***Break" and the return code 20.
DEFTYPE .l

!TestCase{ctrl_c}
  SetSignal_ $1000,$1000
  NPrint "signalled"
  !Check{0,"not_stopped"}
  SetSignal_ 0,$1000
!EndTestCase

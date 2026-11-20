; rundebuglib-ctrlc-console (#43) - an executable compiled with runtime
; error checking cannot be stopped with Ctrl-C: rundebuglib's TRAP #1
; handler, which every statement calls, tests a word ctrlc that nothing
; sets.  The program sends itself Ctrl-C (exec SetSignal of
; SIGBREAKF_CTRL_C) and prints to the console; then it clears the
; signal and prints "not stopped".
; Correct (as a C program on the console): the Print after the signal
; ends the program with "***Break" and the return code 20.
; @fixed #43
; @configs clib
; @pass runtime_errors=on
NPrint "start"
SetSignal_ $1000,$1000
NPrint "signalled"
SetSignal_ 0,$1000
NPrint "not stopped"
End

; rundebuglib-ctrlc-not-console (#43) - the boundaries of the Ctrl-C
; check of an executable compiled with runtime error checking (Print,
; NPrint, Edit$, Edit on the console end the program after Ctrl-C).
; The case sends itself Ctrl-C (exec SetSignal of SIGBREAKF_CTRL_C)
; and then (1) prints into a file (FileOutput): not the console, the
; program goes on; it clears the signal before it goes back to the
; console; the same for reading the line back with Edit$ (FileInput).
; (2) It sends the signal again and consumes it itself
; (SetSignal_ that clears it, as a program that tests Ctrl-C does):
; the next NPrint finds nothing.
; Correct: both parts run to their end, the file holds the line, the
; return code is 0.
; A regression test of the fix of #43: the original never ends such a
; program.

!TestCase{ctrlc_not_console}
  !Require{WriteFile(0,"CORPUS:ctrlc-not-console"),"file_written"}
  FileOutput 0
  SetSignal_ $1000,$1000
  NPrint "into the file"
  SetSignal_ 0,$1000
  CloseFile 0
  DefaultOutput
  NPrint "file done"
  !Require{ReadFile(0,"CORPUS:ctrlc-not-console"),"file_read"}
  FileInput 0
  SetSignal_ $1000,$1000
  a$=Edit$(40)
  SetSignal_ 0,$1000
  CloseFile 0
  DefaultInput
  DefaultOutput
  !CheckStrEq{a$,"into the file","line_read_back"}
  SetSignal_ $1000,$1000
  r.l=SetSignal_(0,$1000)
  NPrint "consumed"
  !CheckEq{r AND $1000,$1000,"signal_consumed"}
!EndTestCase

; rundebuglib-two-programs (#1069) - runtime error checking (rundebuglib,
; also debuglib and errtraplib) puts its handlers into the CPU's trap
; vectors (TRAP #0-#5, the table at the VBR) at the start of a program and
; puts back the vectors it found at its end.  The table belongs to the
; whole machine: with two such programs at once, the one started first
; and ended first puts back the vectors it found, the handlers of a third
; program or of exec, while the other one still runs: that one's traps
; (TRAP #1 before every statement, TRAP #0 for a runtime error) no longer
; reach its own handlers.
; Here the program (the parent) starts itself twice: "a" in the
; background (dos Execute of Run), which waits one second and ends, and
; half a second later "b" in the foreground (dos Execute, output to the
; same file), which waits two seconds - "a" ends meanwhile - and then
; reads an array out of range.  The parent waits for "b".  In the
; original, "a" puts back the parent's handlers, which "b" then reaches:
; the requester is the parent's, and its end of the program runs in the
; process of "b" - the parent never prints "parent done".
; Correct: "b" shows its runtime error requester and ends; then the
; parent ends.
; @fixed #1069
; @pass runtime_errors=on
DEFTYPE .l
Dim a(5)
If NumPars=0
  c$="Run >NIL: CORPUS:out/rundebuglib-two-programs a"
  Execute_ &c$,0,0
  Delay_ 25
  c$="CORPUS:out/rundebuglib-two-programs b"
  Execute_ &c$,0,Output_
  NPrint "parent done"
  End
EndIf
If Par$(1)="a"
  Delay_ 50
  End
EndIf
NPrint "b started"
Delay_ 100
NPrint "b still running"
n=10
a(n)=1
NPrint "b no error"
End

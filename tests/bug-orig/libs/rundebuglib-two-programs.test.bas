; rundebuglib-two-programs (#1069) - runtime error checking (rundebuglib,
; also debuglib and errtraplib) puts its handlers into the CPU's trap
; vectors (TRAP #0-#5, the table at the VBR) at the start of a program and
; puts back the vectors it found at its end.  The table belongs to the
; whole machine: with two such programs at once, the one started first
; and ended first puts back the vectors it found, the handlers of a third
; program or of exec, while the other one still runs: that one's traps
; (TRAP #1 before every statement, TRAP #0 for a runtime error) no longer
; reach its own handlers.
; Here the case (the parent) starts two children of
; rundebuglib-two-programs.test/: "a" in the background (dos Execute of
; Run), which waits one second and ends, and half a second later "b" in
; the foreground (!RunProgram), which waits two seconds - "a" ends
; meanwhile - and then reads an array out of range.  In the original, "a"
; puts back the parent's handlers, which "b" then reaches: the requester
; is the parent's, and its end of the program runs in the process of "b" -
; the parent's case never ends.
; Correct: "b" shows its runtime error requester and ends; then the
; parent's case ends.
!TestCase{two}
  c$="Run >NIL: @TESTDIR@/a"
  Execute_ &c$,0,0
  Delay_ 25
  *r.TLRun=!RunProgram{"@TESTDIR@/b","b"}
  !CheckStrEq{*r\printed,"b started"+Chr$(10)+"b still running"+Chr$(10),"printed"}
!EndTestCase

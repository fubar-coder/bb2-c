; memlib-free-seterr - a runtime error inside a recursion, caught by
; SetErr: the lists of the procedures left open stay; the handler then
; calls procedures again (a recursion to depth 40 and back, three times,
; above the open lists) and ends the program, which frees all lists and
; memlib's list headers.  The runtime error is a TRAP #0 at depth 10, as
; the libraries raise it.  The program runs through inject (track: what
; the process leaves allocated).
; Correct: nothing is left allocated (inject's report, asserted on
; `inject`).  The handler ends the program: the case has no end entry.

DEFTYPE .l

Statement rec{n,s$}
  u$=s$
  If n>0 Then rec{n-1,u$}
End Statement

Statement deep{n,s$}
  u$=s$
  If n=0
    !Check{-1,"depth_10"}
    MOVE.l #0,d0 : TRAP #0
  EndIf
  deep{n-1,u$}
End Statement

!TestCase{runtime_error_caught_by_seterr}
  a$="Hello"
  SetErr
    !Check{-1,"caught"}
    For i=1 To 3 : rec{40,a$} : Next
    !Check{-1,"calls_done"}
    End
  End SetErr
  deep{10,a$}
  !Check{0,"after_deep"}
!EndTestCase

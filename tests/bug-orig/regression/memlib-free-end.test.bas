; memlib-free-end - memlib keeps a list of memory for every running
; procedure (its string parameters and local strings) and the headers of
; these lists; the end of the program frees all of it, also when the
; program ends inside a procedure (End at recursion depth 10).  The
; program runs through inject (track: what the process leaves
; allocated): a recursion to depth 40 and back, three times, 2000 calls at
; one depth, then End at depth 10.
; Correct: nothing is left allocated (inject's report, asserted on
; `inject`).  The program ends inside the procedure: the case has no end
; entry.

DEFTYPE .l

Statement rec{n,s$}
  u$=s$
  If n>0 Then rec{n-1,u$}
End Statement

Statement deep{n,s$}
  u$=s$
  If n=0
    !Check{-1,"depth_10"}
    End
  EndIf
  deep{n-1,u$}
End Statement

!TestCase{end_inside_procedure}
  a$="Hello"
  For i=1 To 3 : rec{40,a$} : Next
  For i=1 To 2000 : rec{0,a$} : Next
  !Check{-1,"calls_done"}
  deep{10,a$}
  !Check{0,"after_end"}
!EndTestCase

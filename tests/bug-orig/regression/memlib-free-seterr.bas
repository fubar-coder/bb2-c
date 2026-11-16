; memlib-free-seterr - a runtime error inside a recursion, caught by
; SetErr: the lists of the procedures left open stay; the handler then
; calls procedures again (a recursion to depth 40 and back, three times,
; above the open lists) and ends the program, which frees all lists and
; memlib's list headers.  The runtime error is a TRAP #0 at depth 10, as
; the libraries raise it.  The program runs through inject (track: what
; the process leaves allocated).
; Correct: nothing is left allocated.
; @pass runwrap=@INJECT@|track
DEFTYPE .l
Statement rec{n,s$}
  u$=s$
  If n>0 Then rec{n-1,u$}
End Statement
Statement deep{n,s$}
  u$=s$
  If n=0
    NPrint "error at depth 10"
    MOVE.l #0,d0 : TRAP #0
  EndIf
  deep{n-1,u$}
End Statement
a$="Hello"
SetErr
  NPrint "caught"
  For i=1 To 3 : rec{40,a$} : Next
  NPrint "calls done"
  End
End SetErr
deep{10,a$}
NPrint "not here"
End

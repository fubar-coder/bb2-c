; memlib-free-end - memlib keeps a list of memory for every running
; procedure (its string parameters and local strings) and the headers of
; these lists; the end of the program frees all of it, also when the
; program ends inside a procedure (End at recursion depth 10).  The
; program runs through inject (track: what the process leaves
; allocated): a recursion to depth 40 and back, three times, 2000 calls at
; one depth, then End at depth 10.
; Correct: nothing is left allocated.
; @pass runwrap=@INJECT@|track
DEFTYPE .l
Statement rec{n,s$}
  u$=s$
  If n>0 Then rec{n-1,u$}
End Statement
Statement deep{n,s$}
  u$=s$
  If n=0 Then NPrint "end at depth 10" : End
  deep{n-1,u$}
End Statement
a$="Hello"
For i=1 To 3 : rec{40,a$} : Next
For i=1 To 2000 : rec{0,a$} : Next
NPrint "calls done"
deep{10,a$}
NPrint "not here"
End

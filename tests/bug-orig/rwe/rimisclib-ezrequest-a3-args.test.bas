; rimisclib-ezrequest-a3-args (#1027) - EZRequest and EZRequestArgs
; pass the argument array to EasyRequestArgs in a3 and return without
; restoring it: a3, the write pointer of the string buffer, is 0 after
; the forms without arguments and the library's argument array after the
; forms with them.  As a function in a string expression the rest of the
; expression is then written there (in the original the program goes
; wrong after the first line).  Each line builds "ab"+Str$(...)+"cd" for
; one form and reads its characters, a 0 as "." (the emulator answers
; every requester with 0).
; Correct: every form gives "5 ab0cd": a3 goes back to the first string
; argument, so neither the arguments nor anything else is in the result.
DEFTYPE .l
Function$ codes{s$}
  t$=""
  For i.l=1 To Len(s$)
    c$=Mid$(s$,i,1)
    If Asc(c$)=0 Then c$="."
    t$=t$+c$
  Next
  Function Return t$
End Function

!TestCase{forms}
  Screen 0,0,0,320,200,2,0,"Workbench",1,2
  a$="ab"+Str$(EZRequest("T","B","OK"))+"cd"
  a1$=codes{a$} : l1=Len(a$)
  a$="ab"+Str$(EZRequest("B","OK"))+"cd"
  a2$=codes{a$} : l2=Len(a$)
  a$="ab"+Str$(EZRequest("B"))+"cd"
  a3$=codes{a$} : l3=Len(a$)
  a$="ab"+Str$(EZRequest("T","B %ld","OK",7))+"cd"
  a4$=codes{a$} : l4=Len(a$)
  a$="ab"+Str$(EZRequestArgs(7))+"cd"
  a5$=codes{a$} : l5=Len(a$)
  a$="ab"+Str$(EZRequestArgs("x","y"))+"cd"
  a6$=codes{a$} : l6=Len(a$)
  !CheckStrEq{a1$,"ab0cd","title_body_gadget"}
  !CheckStrEq{a2$,"ab0cd","body_gadget"}
  !CheckStrEq{a3$,"ab0cd","body"}
  !CheckStrEq{a4$,"ab0cd","with_argument"}
  !CheckStrEq{a5$,"ab0cd","args_long"}
  !CheckStrEq{a6$,"ab0cd","args_strings"}
  !CheckEq{l1+l2+l3+l4+l5+l6,30,"lengths"}
!EndTestCase

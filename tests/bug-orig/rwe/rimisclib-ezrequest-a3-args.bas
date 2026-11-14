; rimisclib-ezrequest-a3-args (#1027) - EZRequest and EZRequestArgs
; pass the argument array to EasyRequestArgs in a3 and return without
; restoring it: a3, the write pointer of the string buffer, is 0 after
; the forms without arguments and the library's argument array after the
; forms with them.  As a function in a string expression the rest of the
; expression is then written there (in the original the program goes
; wrong after the first line).  Each line builds "ab"+Str$(...)+"cd" for
; one form and prints its length and its characters, a 0 as "." (the
; emulator answers every requester with 0).
; Correct: every form gives "5 ab0cd": a3 goes back to the first string
; argument, so neither the arguments nor anything else is in the result.
; @fixed #1027
; @rwe rimisclib
DEFTYPE .l
Statement codes{s$}
  t$=""
  For i.l=1 To Len(s$)
    c$=Mid$(s$,i,1)
    If Asc(c$)=0 Then c$="."
    t$=t$+c$
  Next
  NPrint Len(s$)," ",t$
End Statement
Screen 0,0,0,320,200,2,0,"Workbench",1,2
a$="ab"+Str$(EZRequest("T","B","OK"))+"cd"
codes{a$}
a$="ab"+Str$(EZRequest("B","OK"))+"cd"
codes{a$}
a$="ab"+Str$(EZRequest("B"))+"cd"
codes{a$}
a$="ab"+Str$(EZRequest("T","B %ld","OK",7))+"cd"
codes{a$}
a$="ab"+Str$(EZRequestArgs(7))+"cd"
codes{a$}
a$="ab"+Str$(EZRequestArgs("x","y"))+"cd"
codes{a$}
End

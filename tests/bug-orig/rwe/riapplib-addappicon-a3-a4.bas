; riapplib-addappicon-a3-a4 (#516) - AddAppIcon passes the DiskObject
; to workbench.library's AddAppIconA in a3 and the tag list (0) in a4 and
; restores neither.  a4 is the base of the locals of a Statement or
; Function: a local read after AddAppIcon is read at a small offset from
; 0, and the procedure's UNLK a4 at its end loads the stack pointer from
; 0.  a3 is the write pointer of the string buffer: as a function in a
; string expression the rest of the expression is written into the
; DiskObject.  The icon is the volume's (SYS:Disk); whether Workbench
; takes the App icon does not matter, the registers are set before the
; call.  The program prints the length and the characters of
; "ab"+Str$(AddAppIcon(...))+"cd" (a 0 as "."), then a local after
; AddAppIcon inside a Statement.
; Correct: "ab", the result, "cd"; the local is 7; "done" after the
; Statement.
; @fixed #516
; @rwe riapplib
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
Statement appicon{}
  a.l=7
  r.l=AddAppIcon(2,"T","SYS:Disk")
  NPrint "local ",a
End Statement
a$="ab"+Str$(AddAppIcon(1,"T","SYS:Disk"))+"cd"
codes{a$}
appicon{}
NPrint "done"
End

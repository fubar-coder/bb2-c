; t42_edit - editlib: Edit$, Inkey$, Edit with input from a file (FileInput)
f$="RAM:bb2t42.txt"
If WriteFile(0,f$)
  FileOutput 0
  NPrint "first line"
  NPrint "12.75"
  NPrint "-0.5"
  NPrint "abc"
  NPrint "a longer line with more characters"
  NPrint "xy"
  NPrint ""
  NPrint "$1F"
  NPrint "7"
  NPrint "zz"
  Print "ABCDEFGH"
  PopOutput
  CloseFile 0
EndIf
If ReadFile(0,f$)
  FileInput 0
  a$=Edit$(80) : NPrint "edit$80 [",a$,"] ",Len(a$)
  g.f=Abs(g) : f.f=Edit(10) : NPrint "edit10 [",f,"]"
  g.f=Abs(g) : f.f=Edit(2.5,10) : NPrint "edit2.5 [",f,"]"
  a$=Edit$("default",10) : NPrint "edit$dflt [",a$,"] ",Len(a$)
  a$=Edit$(5) : NPrint "edit$5 [",a$,"] ",Len(a$)
  a$=Edit$("toolongdefault",3) : NPrint "edit$long [",a$,"] ",Len(a$)
  a$=Edit$(4) : NPrint "edit$empty [",a$,"] ",Len(a$)
  g.f=Abs(g) : f.f=Edit(-1E+10,80) : NPrint "edit$1F [",f,"]"
  g.f=Abs(g) : f.f=Edit(1) : NPrint "edit1 [",f,"]"
  b$="x"+Edit$("q",2)+"y" : NPrint "concat [",b$,"]"
  a$=Inkey$ : NPrint "inkey [",a$,"] ",Len(a$)
  a$=Inkey$(3) : NPrint "inkey3 [",a$,"] ",Len(a$)
  a$=Inkey$(1) : NPrint "inkey1 [",a$,"] ",Len(a$)
  a$=Inkey$(10) : NPrint "inkey10 [",a$,"] ",Len(a$)
  a$=Inkey$ : NPrint "inkey eof [",a$,"] ",Len(a$)
  a$=Inkey$(2) : NPrint "inkey2 eof [",a$,"] ",Len(a$)
  PopInput
  CloseFile 0
EndIf
KillFile f$
NPrint "end"
End

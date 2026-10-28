; t06_flow - control flow, data/read
For i=1 To 10 Step 3 : Print i," " : Next : NPrint ""
For i=10 To 1 Step -4 : Print i," " : Next : NPrint ""
For f.f=0 To 1 Step 0.25 : Print f," " : Next : NPrint ""
n=0 : While n<5 : n=n+2 : Wend : NPrint "while ",n
n=0 : Repeat : n=n+3 : Until n>10 : NPrint "repeat ",n
For i=1 To 4
  Select i
    Case 1 : NPrint "one"
    Case 2
      NPrint "two"
    Default
      NPrint "other ",i
  End Select
Next
a$="b"
Select a$
  Case "a" : NPrint "is a"
  Case "b" : NPrint "is b"
End Select
For i=1 To 3
  If i=2 Then NPrint "if then 2"
  If i=1
    NPrint "block 1"
  Else
    NPrint "else ",i
  EndIf
Next
For y=1 To 3 : For x=1 To 3 : Print x*y," " : Next : NPrint "" : Next
Restore mydata
For i=1 To 4 : Read v : Print v," " : Next : NPrint ""
Read s$ : NPrint s$
Restore mydata : Read v : NPrint "again ",v
End
mydata:
Data 11,22,33,44
Data$ "text"

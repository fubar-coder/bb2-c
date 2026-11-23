; a child of model-run-program: 3000 bytes, 100 lines of 30
For i=0 To 99
  a$="line "+Str$(i)+" "
  NPrint a$+String$("x",29-Len(a$))
Next
End

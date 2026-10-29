; t14_select - Select on bytes (the templates selpushb/selcomb), words, longs
For b.b=-2 To 3
  Select b
    Case 0 : NPrint "byte zero"
    Case 2
      NPrint "byte two ",b
    Case -1 : NPrint "byte minus one"
    Default
      NPrint "byte other ",b
  End Select
Next
c.b=200
Select c
  Case 200 : NPrint "byte 200"
  Case -56 : NPrint "byte -56"
  Default : NPrint "byte default ",c
End Select
For w.w=254 To 257
  Select w
    Case 255 : NPrint "word 255"
    Case 256 : NPrint "word 256"
    Default : NPrint "word other ",w
  End Select
Next
l.l=100000
Select l
  Case 100000 : NPrint "long 100000"
  Default : NPrint "long other ",l
End Select
End

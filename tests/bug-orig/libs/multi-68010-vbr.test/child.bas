; a child of multi-68010-vbr: its argument is the address of the moved
; vector table; it prints whether cr_vbr (environlib, read at its start)
; is that table, then sets a SetErr handler and raises TRAP #0
DEFTYPE .l
a$=Par$(1) : p=0
For i=1 To Len(a$) : p=p*10+Asc(Mid$(a$,i,1))-48 : Next
NPrint "cr_vbr is the moved table ",cr_vbr=p
SetErr
  NPrint "SetErr handler reached"
  End
End SetErr
MOVEQ #0,d0
TRAP #0
NPrint "no SetErr handler"
End

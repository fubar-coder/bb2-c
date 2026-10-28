; t05_proc - procedures, recursion, gosub, shared
Statement myshow{a,b}
  NPrint "myshow ",a," ",b," ",a*b
End Statement
Function fact{n}
  If n<=1 Then Function Return 1
  Function Return n*fact{n-1}
End Function
Function.f half{x.f}
  Function Return x/2
End Function
Function$ twice{s$}
  Function Return s$+s$
End Function
g=10
Statement useshared{}
  SHARED g
  g=g+1
End Statement
myshow{6,7}
NPrint "fact ",fact{1}," ",fact{5}," ",fact{10}
NPrint "half ",half{5}
NPrint "twice ",twice{"ab"}
useshared{} : useshared{}
NPrint "shared ",g
Gosub mysub
NPrint "after gosub"
For i=1 To 3
  On i Gosub l1,l2,l3
Next
Goto fin
mysub:
  NPrint "in mysub"
Return
l1: NPrint "on 1" : Return
l2: NPrint "on 2" : Return
l3: NPrint "on 3" : Return
fin:
NPrint "end"
End

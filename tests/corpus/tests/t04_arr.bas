; t04_arr - arrays, lists, NewTypes
Dim a.w(10)
For i=0 To 10 : a(i)=i*i : Next
s=0 : For i=0 To 10 : s=s+a(i) : Next
NPrint "sum ",s
Dim m.l(3,4)
For y=0 To 4 : For x=0 To 3 : m(x,y)=x*10+y : Next : Next
NPrint "m ",m(2,3)," ",m(3,4)," ",m(0,0)
NEWTYPE .point
  x.w
  y.w
  name$
End NEWTYPE
p.point\x=3,4
p\name="pt"
NPrint "point ",p\x," ",p\y," ",p\name
Dim pts.point(5)
For i=0 To 5 : pts(i)\x=i : pts(i)\y=-i : Next
NPrint "pts ",pts(4)\x," ",pts(4)\y
Dim List l.w(10)
For i=1 To 5
  If AddItem(l()) Then l()=i*100
Next
ResetList l()
While NextItem(l())
  NPrint "list ",l()
Wend
End

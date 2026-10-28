; t10_sort - sortlib: Sort, SortUp, SortDown on every array type
Dim b.b(15),w.w(15),l.l(20),q.q(9),f.f(9),s$(9),w2.w(1),l1.l(0),l2.l(1)
x.l=7
For i=0 To 15
  x=(x*75+74) MOD 65537
  b(i)=x MOD 256-128 : w(i)=x-32768
Next
For i=0 To 20 : x=(x*75+74) MOD 65537 : l(i)=(x-30000)*65536+x : Next
l(3)=l(4) : l(7)=-2147483647-1 : l(8)=2147483647
Sort b()
For i=0 To 15 : Print b(i)," " : Next : NPrint ""
Sort w()
For i=0 To 15 : Print w(i)," " : Next : NPrint ""
Sort l() : Gosub showl
SortDown
Sort l() : Gosub showl
Sort b()
For i=0 To 15 : Print b(i)," " : Next : NPrint ""
Sort w()
For i=0 To 15 : Print w(i)," " : Next : NPrint ""
SortUp
For i=0 To 9 : q(i)=(i*37 MOD 10)-4.25 : f(i)=(i*7 MOD 10)/3-1.5 : Next
f(2)=0 : f(5)=-1000000 : f(6)=0.001 : f(7)=f(1)
Sort q()
For i=0 To 9 : Print q(i)," " : Next : NPrint ""
Sort f()
For i=0 To 9 : Print f(i)," " : Next : NPrint ""
SortDown : Sort f() : SortUp
For i=0 To 9 : Print f(i)," " : Next : NPrint ""
s$(0)="pear" : s$(1)="Apple" : s$(2)="" : s$(3)="apple" : s$(4)="ab"
s$(5)="abc" : s$(6)="a" : s$(7)="pear" : s$(8)="zz"+Chr$(200) : s$(9)="zz"
Sort s$()
For i=0 To 9 : Print "[",s$(i),"]" : Next : NPrint ""
SortDown : Sort s$() : SortUp
For i=0 To 9 : Print "[",s$(i),"]" : Next : NPrint ""
; sorted, reverse, all equal, small arrays
For i=0 To 20 : l(i)=i : Next : Sort l() : Gosub showl
For i=0 To 20 : l(i)=-i : Next : Sort l() : Gosub showl
For i=0 To 20 : l(i)=5 : Next : Sort l() : Gosub showl
w2(0)=9 : w2(1)=-9 : Sort w2() : NPrint "w2 ",w2(0)," ",w2(1)
l1(0)=42 : Sort l1() : NPrint "l1 ",l1(0)
l2(0)=3 : l2(1)=-3 : Sort l2() : NPrint "l2 ",l2(0)," ",l2(1)
SortDown : Sort l2() : NPrint "l2 down ",l2(0)," ",l2(1) : SortUp
End
showl:
For i=0 To 20 : Print l(i)," " : Next : NPrint ""
Return

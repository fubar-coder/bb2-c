; divzero-next-compile.test/sum.bas: + - * / MOD on the plain path, + on
; the mixed path (a Quick and a Float), a comparison, a Word * in a later
; array index; compiled after div0.bas and div0index.bas
a.l=3 : b.l=4
c.l=a+b : NPrint c
c=a*b : NPrint c
c=a-b : NPrint c
c=20/b : NPrint c
q.q=1.5 : f.f=2.5
f=q+f : NPrint f
c=10 MOD b : NPrint c
If a<b Then NPrint "lt"
Dim m.w(9,9)
i.w=2 : j.w=3
m(1,i*j)=42 : NPrint m(1,6)
End

; t02_float - floating point (FFP) and conversions
a.f=3.25 : b.f=-0.5
NPrint a+b," ",a-b," ",a*b," ",a/b
NPrint "int ",Int(a)," ",Int(b)," frac ",Frac(a)
NPrint "float mod ",a MOD 2
c.f=1/3 : NPrint "third ",c
d.f=1000000 : NPrint "big ",d*d
NPrint "small ",0.001*0.001
w.w=a*10 : NPrint "f->w ",w
l.l=-a*1000 : NPrint "f->l ",l
q.q=a : NPrint "f->q ",q
e.f=q*2 : NPrint "q->f ",e
NPrint "cmp ",a>b," ",a<b," ",a=3.25
NPrint "sqr ",Sqr(16)," ",Sqr(2)
NPrint "pow ",2^10," ",1.5^2
NPrint "neg ",-a," abs ",Abs(b)
End

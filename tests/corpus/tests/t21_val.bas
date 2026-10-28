; t21_val - vallib: every token, edge cases
Statement pv{s$}
  f.f=Val(s$)
  NPrint "val [",s$,"] ",f," ",Hex$(Peek.l(&f))
End Statement
pv{"42"} : pv{"  42"} : pv{"-42"} : pv{"3.25"} : pv{"-0.5"} : pv{".5"} : pv{"1."}
pv{"$FF"} : pv{"$ff"} : pv{"$1g"} : pv{"$aBc"} : pv{"%1011"} : pv{"%102"} : pv{"abc"}
pv{""} : pv{"   "} : pv{"12abc"} : pv{"1.2.3"} : pv{"-$10"} : pv{"--5"} : pv{"+5"}
pv{"3000000000"} : pv{"4294967295"} : pv{"99999999999"} : pv{"$FFFFFFFF"} : pv{"$7FFFFFFF"}
pv{"%"} : pv{"$"} : pv{"-"} : pv{"0.001"} : pv{"123456.789"} : pv{"- 5"} : pv{" -7.25"}
pv{"1.00000000000000000000000000000001"} : pv{"2.5000000000000000000000000000000000001"}
pv{"%11111111111111111111111111111111"} : pv{"$123456789"} : pv{"$:"} : pv{"$G"} : pv{"1e5"}
f.f=Val(Left$("-12",2)) : NPrint "val left -12,2 ",f
f.f=Val(Left$("$123",2)) : NPrint "val left $123,2 ",f
f.f=Val(Left$("%101",2)) : NPrint "val left %101,2 ",f
f.f=Val(Left$("12",1)) : NPrint "val left 12,1 ",f
f.f=Val(Left$("1.25",3)) : NPrint "val left 1.25,3 ",f
g.f=Val("7")+Val("0.5") : NPrint "val sum ",g
b.b=-5 : w.w=-300 : l.l=-70000 : q.q=-2.5 : f.f=-2.5
NPrint "int ",Int(b)," ",Int(w)," ",Int(l)," ",Int(q)," ",Int(f)
b=100 : w=30000 : l=123456789 : q=2.75 : f=2.75
NPrint "int ",Int(b)," ",Int(w)," ",Int(l)," ",Int(q)," ",Int(f)
q=-0.5 : f=-0.5 : NPrint "int ",Int(q)," ",Int(f)
q=-32768 : NPrint "int q-32768 ",Int(q)
q=32767.9999 : NPrint "int q32767 ",Int(q)
f=1E+10 : NPrint "int big ",Int(f)
f=-1E+10 : NPrint "int -big ",Int(f)
b=-1 : w=-1 : l=-1 : q=-1 : f=-1
NPrint "sgn ",Sgn(b)," ",Sgn(w)," ",Sgn(l)," ",Sgn(q)," ",Sgn(f)
b=0 : w=0 : l=0 : q=0 : f=0
NPrint "sgn ",Sgn(b)," ",Sgn(w)," ",Sgn(l)," ",Sgn(q)," ",Sgn(f)
b=7 : w=7 : l=7 : q=0.25 : f=0.001
NPrint "sgn ",Sgn(b)," ",Sgn(w)," ",Sgn(l)," ",Sgn(q)," ",Sgn(f)
b=-128 : w=-32768 : l=$80000000 : NPrint "sgn min ",Sgn(b)," ",Sgn(w)," ",Sgn(l)
Statement pf{x.f}
  y.f=Frac(x)
  NPrint "frac ",x," ",y," ",Hex$(Peek.l(&y))
End Statement
pf{3.25} : pf{-3.25} : pf{0} : pf{1000000000} : pf{-0.75} : pf{5} : pf{0.1} : pf{-12345.678}
NPrint "qfrac ",QFrac(2.75)," ",QFrac(-2.75)," ",QFrac(0)," ",QFrac(-0.5)," ",QFrac(100)
Print "qwrap ",QWrap(5,0,10)," ",QWrap(-1,0,10)," ",QWrap(10,0,10)," ",QWrap(12,0,10)," "
NPrint QWrap(3.5,1,2)," ",QWrap(-25,0,10)
NPrint "qabs ",QAbs(-3.5)," ",QAbs(3.5)," ",QAbs(0)," ",QAbs(-32768)," ",QAbs(-0.0001)
b=-5 : w=-300 : l=-70000 : q=-2.5 : f=-2.25
x.f=Abs(b) : NPrint "abs b ",x," ",Hex$(Peek.l(&x))
x=Abs(w) : NPrint "abs w ",x," ",Hex$(Peek.l(&x))
x=Abs(l) : NPrint "abs l ",x," ",Hex$(Peek.l(&x))
x=Abs(q) : NPrint "abs q ",x," ",Hex$(Peek.l(&x))
x=Abs(f) : NPrint "abs f ",x," ",Hex$(Peek.l(&x))
q=0 : x=Abs(q) : NPrint "abs q0 ",x," ",Hex$(Peek.l(&x))
q=1.5 : x=Abs(q) : NPrint "abs q+ ",x," ",Hex$(Peek.l(&x))
l=$80000000 : x=Abs(l) : NPrint "abs lmin ",x," ",Hex$(Peek.l(&x))
Print "qlimit ",QLimit(5,0,10)," ",QLimit(-5,0,10)," ",QLimit(10,0,10)," ",QLimit(15,0,10)," "
NPrint QLimit(5,10,0)," ",QLimit(0,0,10)
NPrint "true ",True," false ",False," ntsc ",NTSC," dispheight ",DispHeight
v.w=VPos : ok=0 : If v>=0 AND v<512 Then ok=1
NPrint "vpos ok ",ok
bad=0 : zero=0
For i=1 To 200
  r.q=Rnd : If r<0 OR r>=1 Then bad=bad+1
  r=Rnd(10) : If r<0 OR r>=10 Then bad=bad+1
  r=Rnd(0) : If r<>0 Then zero=zero+1
  r=Rnd(1) : If r<0 OR r>=1 Then bad=bad+1
Next
NPrint "rnd bad ",bad," zero ",zero
Statement pa{x.w,y.w,u.w,v.w}
  a.q=QAngle(x,y,u,v)
  NPrint "qangle ",x," ",y," ",u," ",v," ",Hex$(Peek.l(&a))
End Statement
pa{0,0,10,0} : pa{0,0,0,10} : pa{0,0,-10,0} : pa{0,0,0,-10} : pa{0,0,10,10} : pa{0,0,-10,10}
pa{0,0,10,-10} : pa{0,0,-10,-10} : pa{0,0,3,7} : pa{0,0,7,3} : pa{0,0,-3,7} : pa{0,0,-7,3}
pa{0,0,3,-7} : pa{0,0,-7,-3} : pa{5,5,5,5} : pa{0,0,-32768,0} : pa{0,0,0,-32768} : pa{0,0,-32768,-32768}
pa{100,50,-20,13} : pa{-300,200,123,-456} : pa{0,0,1,1000} : pa{0,0,1000,1} : pa{0,0,32767,-32768}
pa{0,0,-32768,1} : pa{0,0,1,-32768} : pa{0,0,32767,32767} : pa{10,10,0,0}
NPrint "end"
End

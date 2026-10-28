; t11_math - mathtranslib: transcendental functions and ^ on floats
Dim v.f(8)
v(0)=0 : v(1)=0.5 : v(2)=-0.5 : v(3)=1 : v(4)=-1 : v(5)=2 : v(6)=3.14159 : v(7)=-10 : v(8)=100
For i=0 To 8
  x.f=v(i)
  NPrint "x=",x
  NPrint " acos ",ACos(x)," asin ",ASin(x)," atan ",ATan(x)
  NPrint " cos ",Cos(x)," sin ",Sin(x)," tan ",Tan(x)
  NPrint " hcos ",HCos(x)," hsin ",HSin(x)," htan ",HTan(x)
  NPrint " exp ",Exp(x)," log ",Log(x)," log10 ",Log10(x)," sqr ",Sqr(x)
Next
a.f=2 : b.f=10 : NPrint "pow ",a^b," ",b^a," ",a^0.5," ",a^-1
a=0 : NPrint "0^0 ",a^0," 0^2 ",a^2
a=-2 : NPrint "neg ",a^2," ",a^3," ",a^0.5
a=10 : NPrint "big ",a^18," ",a^19," ",a^-19
NPrint "expr ",Sin(1)*Sin(1)+Cos(1)*Cos(1)," ",Exp(Log(5))," ",Sqr(2)*Sqr(2)
NPrint "nested ",ATan(Tan(0.7))," ",Log10(10^3)
y.l=Sqr(1000) : NPrint "to long ",y
q.q=Sin(0.5) : NPrint "to quick ",q
End

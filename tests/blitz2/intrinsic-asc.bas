; @runerrs off
; intrinsic-asc - Asc of a plain string variable reads the first character
; from the variable's memory (compiler-intrinsics, openspec/specs/
; compiler/intrinsics): no copy into the work area, no library call, the
; value of the library (the first character zero-extended, -1 for an empty
; string and a variable without memory).  The same program with runtime
; error checking: intrinsic-asc-runerrs.bas.  The values are those of the
; build before the intrinsic (the copy and the library's Asc).
; jsrs{a,b}: the JSR instructions ($4EB9, every library call and the copy
; copstring) of the code from label a to label b
Function.l jsrs{a.l,b.l}
  n.l=0
  While a<b
    If Peek.w(a)=$4EB9
      n=n+1 : a=a+4
    EndIf
    a=a+2
  Wend
  Function Return n
End Function
; a parameter, a local, a Shared global, an unassigned local
Function.l pasc{s$}
  Shared g$
  t$="!"+s$
  Function Return Asc(s$)+Asc(t$)*1000+Asc(g$)*1000000+Asc(u$)
End Function
DEFTYPE .l
c=0 : d=7
a$="hello"
.a3
c=d
.a4
.a1
c=Asc(a$)
.a2
NPrint "c=Asc(a$) ",c," extra calls ",jsrs{?a1,?a2}-jsrs{?a3,?a4}
.b1
NPrint Asc(a$)
.b2
NPrint "NPrint Asc(a$) calls ",jsrs{?b1,?b2}
; the first characters 0, 127, 128 and 255 (zero-extended), "",
; unassigned, a one character string, after a longer one
a$=Chr$(0)+"x" : c=Asc(a$) : NPrint "0 ",c
a$=Chr$(127)+"x" : c=Asc(a$) : NPrint "127 ",c
a$=Chr$(128)+"x" : c=Asc(a$) : NPrint "128 ",c
a$=Chr$(255)+"x" : c=Asc(a$) : NPrint "255 ",c
a$=Chr$(255) : w.w=Asc(a$) : NPrint "255 as a Word ",w
a$="" : c=Asc(a$) : NPrint "empty ",c
a$="" : w=Asc(a$) : NPrint "empty as a Word ",w
c=Asc(z$) : NPrint "unassigned ",c
a$="hello" : a$="" : c=Asc(a$) : NPrint "emptied ",c
a$="A" : c=Asc(a$) : NPrint "A ",c
a$=String$("q",10000) : c=Asc(a$) : NPrint "10000 ",c
; nested in a string expression (a3 the end of the string being built),
; two in one expression, an operand with a partial result in d0, an
; argument of a function, a condition
a$="AB" : b$="cde"
.c3
b2$=a$+Chr$(d)
.c4
.c1
b2$=a$+Chr$(Asc(a$)+1)
.c2
NPrint "b2$=a$+Chr$(Asc(a$)+1) ",b2$," extra calls ",jsrs{?c1,?c2}-jsrs{?c3,?c4}
.d3
c=d+d
.d4
.d1
c=Asc(a$)+Asc(b$)
.d2
NPrint "c=Asc(a$)+Asc(b$) ",c," extra calls ",jsrs{?d1,?d2}-jsrs{?d3,?d4}
c=d*3+Asc(b$)*2 : NPrint "c=d*3+Asc(b$)*2 ",c
b2$=b$+Hex$(Asc( b$ ))+a$ : NPrint "b2$=b$+Hex$(Asc( b$ ))+a$ ",b2$
If Asc(a$)=65 Then NPrint "If Asc(a$)=65"
g$="global"
c=pasc{"param"} : NPrint "pasc ",c
End

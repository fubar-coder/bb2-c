; @runerrs off
; intrinsic-len - Len of a plain string variable reads the length from the
; variable's memory (compiler-intrinsics, openspec/specs/compiler/
; intrinsics): no copy into the work area, no library call, the value of
; the library (the length at -4 of the characters, 0 for a variable
; without memory).  The same program with runtime error checking:
; intrinsic-len-runerrs.bas.  The values are those of the build before
; the intrinsic (the copy and the library's Len).
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
Function.l plen{s$}
  Shared g$
  t$=s$+"!"
  Function Return Len(s$)+Len(t$)*100+Len(g$)*10000+Len(u$)*1000000
End Function
DEFTYPE .l
c=0 : d=7
; the forms: a short string, "", unassigned, a long string, characters 0
a$="hello"
.a3
c=d
.a4
.a1
c=Len(a$)
.a2
NPrint "c=Len(a$) ",c," extra calls ",jsrs{?a1,?a2}-jsrs{?a3,?a4}
.b1
NPrint Len(a$)
.b2
NPrint "NPrint Len(a$) calls ",jsrs{?b1,?b2}
a$="" : c=Len(a$) : NPrint "empty ",c
c=Len(z$) : NPrint "unassigned ",c
a$=String$("x",10000) : c=Len(a$) : NPrint "10000 ",c
a$=Chr$(0)+"x"+Chr$(0) : c=Len(a$) : NPrint "zeros ",c
a$="hello" : a$="hi" : c=Len(a$) : NPrint "shorter ",c
; nested in a string expression (a3 the end of the string being built),
; two in one expression, an operand with a partial result in d0, an
; argument of a function, spaces, a condition
a$="AB" : b$="cde"
.c3
b2$=a$+Chr$(d)
.c4
.c1
b2$=a$+Chr$(64+Len(a$))
.c2
NPrint "b2$=a$+Chr$(64+Len(a$)) ",b2$," extra calls ",jsrs{?c1,?c2}-jsrs{?c3,?c4}
.d3
c=d+d
.d4
.d1
c=Len(a$)+Len(b$)
.d2
NPrint "c=Len(a$)+Len(b$) ",c," extra calls ",jsrs{?d1,?d2}-jsrs{?d3,?d4}
c=d*3+Len(b$)*2 : NPrint "c=d*3+Len(b$)*2 ",c
b2$=Left$(b$,Len( a$ )) : NPrint "b2$=Left$(b$,Len( a$ )) ",b2$
b2$=b$+Str$(Len(b$))+a$ : NPrint "b2$=b$+Str$(Len(b$))+a$ ",b2$
If Len(a$)=2 Then NPrint "If Len(a$)=2"
g$="global"
c=plen{"param"} : NPrint "plen ",c
; the length is a long: the header of a string of 10000 set to 70000 for
; one Len (the characters are not read), then back.  The build before
; gives the same value, but copies the 70000 characters into the work
; area of 10240 bytes, past its end.
a$=String$("y",10000) : p.l=&a$ : Poke.l p-4,70000
c=Len(a$)
Poke.l p-4,10000
NPrint "header 70000 ",c," then ",Len(a$)
End

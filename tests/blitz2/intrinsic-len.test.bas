; intrinsic-len - Len of a plain string variable reads the length from the
; variable's memory (compiler-intrinsics, openspec/specs/compiler/
; intrinsics): no copy into the work area, no library call, the value of
; the library (the length at -4 of the characters, 0 for a variable
; without memory).  Without and with runtime error checking (the axis
; runtime-errors of intrinsic-len.test.toml): the statement NPrint Len(a$)
; has one JSR more with it.  The values are those of the build before the
; intrinsic (the copy and the library's Len).
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
; runerrs{a,b}: 1 if the code from label a to label b holds a trap #1
; ($4E41, the debugger's step in front of every statement): the program
; is compiled with runtime error checking
Function.l runerrs{a.l,b.l}
  While a<b
    If Peek.w(a)=$4E41 Then Function Return 1
    a=a+2
  Wend
  Function Return 0
End Function
; a parameter, a local, a Shared global, an unassigned local
Function.l plen{s$}
  Shared g$
  t$=s$+"!"
  Function Return Len(s$)+Len(t$)*100+Len(g$)*10000+Len(u$)*1000000
End Function
DEFTYPE .l
; rv{a,b}: a without runtime error checking, b with it
Function.l rv{a.l,b.l}
  Shared re
  If re Then Function Return b
  Function Return a
End Function

!TestBatchInit
.re1
  re=0
.re2
  re=runerrs{?re1,?re2}
!EndTestBatchInit

!TestCase{calls}
  c=0 : d=7
  a$="hello"
.a3
  c=d
.a4
.a1
  c=Len(a$)
.a2
  !CheckEq{c,5,"value"}
  !CheckEq{jsrs{?a1,?a2}-jsrs{?a3,?a4},0,"extra_calls"}
.b1
  NPrint Len(a$)
.b2
  !CheckEq{jsrs{?b1,?b2},rv{1,2},"nprint_calls"}
!EndTestCase

; the forms: "", unassigned, a long string, characters 0, after a longer one
!TestCase{values}
  a$="" : c=Len(a$) : !CheckEq{c,0,"empty"}
  c=Len(z$) : !CheckEq{c,0,"unassigned"}
  a$=String$("x",10000) : c=Len(a$) : !CheckEq{c,10000,"long"}
  a$=Chr$(0)+"x"+Chr$(0) : c=Len(a$) : !CheckEq{c,3,"zeros"}
  a$="hello" : a$="hi" : c=Len(a$) : !CheckEq{c,2,"shorter"}
!EndTestCase

; nested in a string expression (a3 the end of the string being built),
; two in one expression, an operand with a partial result in d0, an
; argument of a function, spaces, a condition, the forms of plen
!TestCase{expressions}
  d=7
  a$="AB" : b$="cde"
.c3
  b2$=a$+Chr$(d)
.c4
.c1
  b2$=a$+Chr$(64+Len(a$))
.c2
  !CheckStrEq{b2$,"ABB","nested"}
  !CheckEq{jsrs{?c1,?c2}-jsrs{?c3,?c4},0,"nested_extra_calls"}
.d3
  c=d+d
.d4
.d1
  c=Len(a$)+Len(b$)
.d2
  !CheckEq{c,5,"two"}
  !CheckEq{jsrs{?d1,?d2}-jsrs{?d3,?d4},0,"two_extra_calls"}
  c=d*3+Len(b$)*2 : !CheckEq{c,27,"partial"}
  b2$=Left$(b$,Len( a$ )) : !CheckStrEq{b2$,"cd","spaces"}
  b2$=b$+Str$(Len(b$))+a$ : !CheckStrEq{b2$,"cde3AB","argument"}
  f=0 : If Len(a$)=2 Then f=1
  !CheckEq{f,1,"condition"}
  g$="global"
  c=plen{"param"} : !CheckEq{c,60605,"procedure"}
!EndTestCase

; the length is a long: the header of a string of 10000 set to 70000 for
; one Len (the characters are not read), then back.  The build before
; gives the same value, but copies the 70000 characters into the work
; area of 10240 bytes, past its end.
!TestCase{long_header}
  a$=String$("y",10000) : p.l=&a$ : Poke.l p-4,70000
  c=Len(a$)
  Poke.l p-4,10000
  !CheckEq{c,70000,"header"}
  !CheckEq{Len(a$),10000,"restored"}
!EndTestCase

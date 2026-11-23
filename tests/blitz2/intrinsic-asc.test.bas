; intrinsic-asc - Asc of a plain string variable reads the first character
; from the variable's memory (compiler-intrinsics, openspec/specs/
; compiler/intrinsics): no copy into the work area, no library call, the
; value of the library (the first character zero-extended, -1 for an empty
; string and a variable without memory).  Without and with runtime error
; checking (the axis runtime-errors of intrinsic-asc.test.toml): the
; statement NPrint Asc(a$) has one JSR more with it.  The values are those
; of the build before the intrinsic (the copy and the library's Asc).
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
Function.l pasc{s$}
  Shared g$
  t$="!"+s$
  Function Return Asc(s$)+Asc(t$)*1000+Asc(g$)*1000000+Asc(u$)
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
  c=Asc(a$)
.a2
  !CheckEq{c,104,"value"}
  !CheckEq{jsrs{?a1,?a2}-jsrs{?a3,?a4},0,"extra_calls"}
.b1
  NPrint Asc(a$)
.b2
  !CheckEq{jsrs{?b1,?b2},rv{1,2},"nprint_calls"}
!EndTestCase

; the first characters 0, 127, 128 and 255 (zero-extended), "",
; unassigned, a one character string, after a longer one
!TestCase{values}
  a$=Chr$(0)+"x" : c=Asc(a$) : !CheckEq{c,0,"char_0"}
  a$=Chr$(127)+"x" : c=Asc(a$) : !CheckEq{c,127,"char_127"}
  a$=Chr$(128)+"x" : c=Asc(a$) : !CheckEq{c,128,"char_128"}
  a$=Chr$(255)+"x" : c=Asc(a$) : !CheckEq{c,255,"char_255"}
  a$=Chr$(255) : w.w=Asc(a$) : !CheckEq{w,255,"word_255"}
  a$="" : c=Asc(a$) : !CheckEq{c,-1,"empty"}
  a$="" : w=Asc(a$) : !CheckEq{w,-1,"word_empty"}
  c=Asc(z$) : !CheckEq{c,-1,"unassigned"}
  a$="hello" : a$="" : c=Asc(a$) : !CheckEq{c,-1,"emptied"}
  a$="A" : c=Asc(a$) : !CheckEq{c,65,"one_char"}
  a$=String$("q",10000) : c=Asc(a$) : !CheckEq{c,113,"long"}
!EndTestCase

; nested in a string expression (a3 the end of the string being built),
; two in one expression, an operand with a partial result in d0, an
; argument of a function, a condition, the forms of pasc
!TestCase{expressions}
  d=7
  a$="AB" : b$="cde"
.c3
  b2$=a$+Chr$(d)
.c4
.c1
  b2$=a$+Chr$(Asc(a$)+1)
.c2
  !CheckStrEq{b2$,"ABB","nested"}
  !CheckEq{jsrs{?c1,?c2}-jsrs{?c3,?c4},0,"nested_extra_calls"}
.d3
  c=d+d
.d4
.d1
  c=Asc(a$)+Asc(b$)
.d2
  !CheckEq{c,164,"two"}
  !CheckEq{jsrs{?d1,?d2}-jsrs{?d3,?d4},0,"two_extra_calls"}
  c=d*3+Asc(b$)*2 : !CheckEq{c,219,"partial"}
  b2$=b$+Hex$(Asc( b$ ))+a$ : !CheckStrEq{b2$,"cde00000063AB","argument"}
  f=0 : If Asc(a$)=65 Then f=1
  !CheckEq{f,1,"condition"}
  g$="global"
  c=pasc{"param"} : !CheckEq{c,103033111,"procedure"}
!EndTestCase

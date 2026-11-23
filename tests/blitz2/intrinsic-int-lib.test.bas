; intrinsic-int-lib - Int where it is not an intrinsic (compiler-intrinsics,
; openspec/specs/compiler/intrinsics): a Float, Long, Word or Byte
; argument compiles the library call as before ("calls 1": the JSR of
; Int; with runtime error checking "calls 2": the JSRs of the error check
; intchk and of Int), with the library's values; a Quick expression and an
; operand with a partial result in a register are intrinsics ("calls 0").
; Without and with runtime error checking (the axis runtime-errors of
; intrinsic-int-lib.test.toml); with it also a string argument: the
; runtime error "Illegal Type" of intchk, as before.
; jsrs{a,b}: the JSR instructions ($4EB9, every library call) of the code
; from label a to label b
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
  c.l=0 : f.f=-2.75 : l.l=-70000 : w.w=-300 : b.b=-5 : q.q=-1.75 : q1.q=2.5
.a1
  c=Int(f)
.a2
  !CheckEq{c,-2,"float"}
  !CheckEq{jsrs{?a1,?a2},rv{1,2},"float_calls"}
  f=2.75
  c=Int(f) : !CheckEq{c,2,"float_positive"}
.b1
  c=Int(l)
.b2
  !CheckEq{c,-70000,"long"}
  !CheckEq{jsrs{?b1,?b2},rv{1,2},"long_calls"}
.c1
  c=Int(w)
.c2
  !CheckEq{c,-300,"word"}
  !CheckEq{jsrs{?c1,?c2},rv{1,2},"word_calls"}
.d1
  c=Int(b)
.d2
  !CheckEq{c,-5,"byte"}
  !CheckEq{jsrs{?d1,?d2},rv{1,2},"byte_calls"}
  ; a Quick expression: an intrinsic (the argument is in d0 either way)
.e1
  c=Int(q-q1)
.e2
  !CheckEq{c,-4,"quick_expression"}
  !CheckEq{jsrs{?e1,?e2},0,"quick_expression_calls"}
  ; a partial result in d0 while the argument is compiled (movem d0)
  c=10
.g1
  c=c+Int(q)
.g2
  !CheckEq{c,9,"partial_quick"}
  !CheckEq{jsrs{?g1,?g2},0,"partial_quick_calls"}
  f=2.75
.h1
  c=c+Int(f)
.h2
  !CheckEq{c,11,"partial_float"}
  !CheckEq{jsrs{?h1,?h2},rv{1,2},"partial_float_calls"}
!EndTestCase

; with runtime error checking a string argument is the runtime error
; "Illegal Type" of intchk (without it the argument is not checked: not run)
!TestCase{string_argument}
  If re
    a$="1.5"
    !ExpectError{"Illegal Type"}
    c=Int(a$)
  EndIf
!EndTestCase

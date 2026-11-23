; intrinsic-len-lib - Len and Asc where they are not intrinsics (compiler-
; intrinsics, openspec/specs/compiler/intrinsics): an argument that is not
; a plain string variable is built in the work area and the library is
; called, as before ("calls": the JSRs of the statement, the copies
; copstring and the calls of the libraries; Len itself is inline code of
; the library, two instructions without a JSR).  The cases: an
; expression, a function result, a literal, an array element, the current
; element of a List, a field of a NewType variable, a field through
; UsePath, a string variable name with a type suffix .s.  The counts and
; values are those of the build before the intrinsics.  Without and with
; runtime error checking (the axis runtime-errors of
; intrinsic-len-lib.test.toml): the array element and the List element
; have the checks of the access too.
; jsrs{a,b}: the JSR instructions ($4EB9) of the code from label a to
; label b
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
NEWTYPE .rec
  id.w
  name$
End NEWTYPE
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
  c=0
  a$="hello" : b$="world!"
  Dim s$(5) : s$(3)="three"
  Dim List ls$(5) : If AddItem(ls$()) Then ls$()="item"
  t.rec\name="field"
  v.s="dot"
.a1
  c=Len(a$+b$)
.a2
  !CheckEq{c,11,"len_expression"}
  !CheckEq{jsrs{?a1,?a2},2,"len_expression_calls"}
.b1
  c=Len(Left$(a$,2))
.b2
  !CheckEq{c,2,"len_function"}
  !CheckEq{jsrs{?b1,?b2},2,"len_function_calls"}
.c1
  c=Len("literal")
.c2
  !CheckEq{c,7,"len_literal"}
  !CheckEq{jsrs{?c1,?c2},1,"len_literal_calls"}
.d1
  c=Len(s$(3))
.d2
  !CheckEq{c,5,"len_element"}
  !CheckEq{jsrs{?d1,?d2},rv{1,3},"len_element_calls"}
.e1
  c=Len(ls$())
.e2
  !CheckEq{c,4,"len_list"}
  !CheckEq{jsrs{?e1,?e2},rv{1,2},"len_list_calls"}
.f1
  c=Len(t\name)
.f2
  !CheckEq{c,5,"len_field"}
  !CheckEq{jsrs{?f1,?f2},1,"len_field_calls"}
  USEPATH t
.g1
  c=Len(\name)
.g2
  !CheckEq{c,5,"len_usepath"}
  !CheckEq{jsrs{?g1,?g2},1,"len_usepath_calls"}
.h1
  c=Len(v)
.h2
  !CheckEq{c,3,"len_suffix"}
  !CheckEq{jsrs{?h1,?h2},1,"len_suffix_calls"}
.i1
  c=Asc(a$+b$)
.i2
  !CheckEq{c,104,"asc_expression"}
  !CheckEq{jsrs{?i1,?i2},3,"asc_expression_calls"}
.j1
  c=Asc(Mid$(b$,2))
.j2
  !CheckEq{c,111,"asc_function"}
  !CheckEq{jsrs{?j1,?j2},3,"asc_function_calls"}
.k1
  c=Asc("literal")
.k2
  !CheckEq{c,108,"asc_literal"}
  !CheckEq{jsrs{?k1,?k2},2,"asc_literal_calls"}
.l1
  c=Asc(s$(3))
.l2
  !CheckEq{c,116,"asc_element"}
  !CheckEq{jsrs{?l1,?l2},rv{2,4},"asc_element_calls"}
.m1
  c=Asc(ls$())
.m2
  !CheckEq{c,105,"asc_list"}
  !CheckEq{jsrs{?m1,?m2},rv{2,3},"asc_list_calls"}
.n1
  c=Asc(t\name)
.n2
  !CheckEq{c,102,"asc_field"}
  !CheckEq{jsrs{?n1,?n2},2,"asc_field_calls"}
.o1
  c=Asc(\name)
.o2
  !CheckEq{c,102,"asc_usepath"}
  !CheckEq{jsrs{?o1,?o2},2,"asc_usepath_calls"}
.p1
  c=Asc(v)
.p2
  !CheckEq{c,100,"asc_suffix"}
  !CheckEq{jsrs{?p1,?p2},2,"asc_suffix_calls"}
  s$(4)="" : c=Len(s$(4)) : !CheckEq{c,0,"len_empty_element"}
  !CheckEq{Asc(s$(4)),-1,"asc_empty_element"}
  c=Len(s$(5)) : !CheckEq{c,0,"len_unassigned_element"}
  !CheckEq{Asc(s$(5)),-1,"asc_unassigned_element"}
!EndTestCase

; array-index-code - the code of a later array index (#1179, spec
; compiler/expression-types, "The index code of a plain later index does
; not grow"): a later index is a Long expression; without runtime error
; checking the upper word of a one-element index is not built (makelmul
; reads only its low word), so a(iw,jw), m(5,7), b(iw,i2,i3) and a(iq,jq)
; have the code size of the Word evaluation of the original (the byte
; counts below); a * or / of two Word values calls no library routine (in
; both builds), a Long * still does.  A folded constant (m(5,3+4),
; m(5,100000-99993)) keeps the original's move.l of the fold (its size
; checked without runtime error checking only).  With runtime error
; checking (the axis runtime-errors of array-index-code.test.toml) a
; later index is a Long expression with the size check (arrszchk); the
; code of a(iw,jw), m(5,7), b(iw,i2,i3) and a(iq,jq) keeps its size, a *
; or / of two Word values calls no library routine (muls.w; divs.w with
; the fall-back for -32768 / -1) where the original called lmullib and
; ldivlib, a Long * still does; the other calls are the checks of the
; access.
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

!TestCase{index_code}
  Dim a.b(1,60000),m.b(9,9),b.b(9,9,9)
  a0=&a(0,0) : m0=&m(0,0) : b0=&b(0,0,0) : o=0
  iw.w=1 : jw.w=300 : kw.w=200 : wn.w=-32768 : wm.w=-1
  il.l=1 : jl.l=300 : kl.l=200 : iq.q=1 : jq.q=2 : i2.w=2 : i3.w=3
.a1
  o=&a(iw,jw)
.a2
  !CheckEq{(o-a0-1)/2,300,"word"}
  !CheckEq{?a2-?a1,rv{44,116},"word_bytes"}
.b1
  o=&m(5,7)
.b2
  !CheckEq{o-m0,75,"constant"}
  !CheckEq{?b2-?b1,rv{44,116},"constant_bytes"}
.b3
  o=&m(5,3+4)
.b4
  !CheckEq{o-m0,75,"fold"}
  If re=0
    !CheckEq{?b4-?b3,46,"fold_bytes"}
  EndIf
.b5
  o=&m(5,100000-99993)
.b6
  !CheckEq{o-m0,75,"fold_long"}
  If re=0
    !CheckEq{?b6-?b5,46,"fold_long_bytes"}
  EndIf
.c1
  o=&b(iw,i2,i3)
.c2
  !CheckEq{o-b0,321,"three"}
  !CheckEq{?c2-?c1,rv{70,174},"three_bytes"}
.d1
  o=&a(iq,jq)
.d2
  !CheckEq{(o-a0-1)/2,2,"quick"}
  !CheckEq{?d2-?d1,rv{52,124},"quick_bytes"}
.e1
  o=&a(iw,jw*kw)
.e2
  !CheckEq{(o-a0-1)/2,60000,"word_times"}
  !CheckEq{jsrs{?e1,?e2},rv{0,4},"word_times_calls"}
.f1
  o=&a(iw,jw/2)
.f2
  !CheckEq{(o-a0-1)/2,150,"word_divide"}
  !CheckEq{jsrs{?f1,?f2},rv{0,4},"word_divide_calls"}
.g1
  o=&a(iw,wn/wm)
.g2
  !CheckEq{(o-a0-1)/2,32768,"word_divide_min"}
  !CheckEq{jsrs{?g1,?g2},rv{0,4},"word_divide_min_calls"}
.h1
  o=&a(il,jl*kl)
.h2
  !CheckEq{(o-a0-1)/2,60000,"long_times"}
  !CheckEq{jsrs{?h1,?h2},rv{1,5},"long_times_calls"}
!EndTestCase

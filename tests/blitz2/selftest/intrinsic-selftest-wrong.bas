; @runerrs off
; @want int f: calls 1
; intrinsic-selftest-wrong - wrong on purpose: the check of the compiled
; code of the intrinsic tests (tests/blitz2/intrinsic-*.bas).  Int of a
; Float is always a library call, the expectation says it is not; the
; runner must report REGRESSED and the report must name the case (the
; observed line "int f: calls 1").
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
f.f=-1.75 : c.l=0
.f1
c=Int(f)
.f2
NPrint "int f: ",c
NPrint "int f: calls ",jsrs{?f1,?f2}
End

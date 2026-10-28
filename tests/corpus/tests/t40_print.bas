; t40_print - printlib: Print/NPrint of every type, Str$, UStr$, Format, FloatMode
Statement pf{x.l}
  f.f=0 : Poke.l &f,x
  NPrint "ffp ",Hex$(x)," [",f,"] [",Str$(f),"]"
End Statement
Statement pq{x.l}
  q.q=0 : Poke.l &q,x
  NPrint "quick ",Hex$(x)," [",q,"] [",Str$(q),"]"
End Statement
Statement fm{f.f}
  Print "[",f,"]"
End Statement
; ---- bytes, words, longs
b.b=0 : NPrint "byte [",b,"]"
b=1 : NPrint "byte [",b,"]"
b=-1 : NPrint "byte [",b,"]"
b=127 : NPrint "byte [",b,"]"
b=-128 : NPrint "byte [",b,"] [",Str$(b),"] ",Len(Str$(b))
w.w=0 : NPrint "word [",w,"]"
w=32767 : NPrint "word [",w,"]"
w=-32768 : NPrint "word [",w,"] [",Str$(w),"]"
w=-1000 : NPrint "word [",w,"]"
w=10 : NPrint "word [",w,"]"
l.l=0 : NPrint "long [",l,"]"
l=-1 : NPrint "long [",l,"]"
l=2147483647 : NPrint "long [",l,"] [",Str$(l),"]"
l=$80000000 : NPrint "long [",l,"] [",Str$(l),"] ",Len(Str$(l))
l=1000000000 : NPrint "long [",l,"]"
l=999999999 : NPrint "long [",l,"]"
l=-100 : NPrint "long [",l,"]"
l=4096 : NPrint "long [",l,"]"
; ---- quick
q.q=0 : NPrint "quick [",q,"]"
q=0.5 : NPrint "quick [",q,"]"
q=-0.5 : NPrint "quick [",q,"]"
q=1 : NPrint "quick [",q,"]"
q=-1 : NPrint "quick [",q,"]"
q=1.25 : NPrint "quick [",q,"]"
q=-1.25 : NPrint "quick [",q,"]"
q=3.14159 : NPrint "quick [",q,"]"
q=100.1 : NPrint "quick [",q,"]"
q=32767.9999 : NPrint "quick [",q,"]"
q=-32768 : NPrint "quick [",q,"]"
pq{$00010001} : pq{$0002FFFF} : pq{$00000001} : pq{$FFFFFFFF} : pq{$80000000}
pq{$7FFFFFFF} : pq{$00008000} : pq{$FFFF8000} : pq{$0000199A} : pq{$000A0CCD}
NPrint "quick str [",Str$(q),"] [",UStr$(-2.75),"]"
; ---- float
f.f=0 : NPrint "float [",f,"]"
f=1 : NPrint "float [",f,"]"
f=-1 : NPrint "float [",f,"]"
f=0.5 : NPrint "float [",f,"]"
f=-0.25 : NPrint "float [",f,"]"
f=123.456 : NPrint "float [",f,"]"
f=1234567 : NPrint "float [",f,"]"
f=12345678 : NPrint "float [",f,"]"
f=99999999 : NPrint "float [",f,"]"
f=100000000 : NPrint "float [",f,"]"
f=1E+10 : NPrint "float [",f,"]"
f=-3.5E+15 : NPrint "float [",f,"]"
f=1E+18 : NPrint "float [",f,"]"
f=9E+18 : NPrint "float [",f,"]"
f=0.001 : NPrint "float [",f,"]"
f=1E-05 : NPrint "float [",f,"]"
f=1.5E-08 : NPrint "float [",f,"]"
f=1E-09 : NPrint "float [",f,"]"
f=-2.5E-12 : NPrint "float [",f,"]"
g.f=1 : h.f=3 : f=g/h : NPrint "float 1/3 [",f,"] [",Str$(f),"]"
f=2/h : NPrint "float 2/3 [",f,"]"
f=10/h : NPrint "float 10/3 [",f,"]"
pf{$80000041} : pf{$80000080} : pf{$FFFFFF7F} : pf{$FFFFFFFF} : pf{$80000001}
pf{$C0000000} : pf{$A0000044} : pf{$CCCCCD3D} : pf{$9502F95E} : pf{$00000041}
NPrint "float str [",Str$(1.5),"] [",UStr$(f),"] ",Len(Str$(f))
; ---- FloatMode
FloatMode 1
NPrint "fm1"
fm{0} : fm{1} : fm{-1} : fm{1.5} : fm{100} : fm{123.456} : fm{-0.001} : fm{1E+10} : fm{2.5E-12}
NPrint ""
NPrint "fm1 str [",Str$(f),"]"
FloatMode -1
NPrint "fm-1"
fm{0} : fm{1} : fm{-1} : fm{1.5} : fm{100} : fm{123.456} : fm{-0.001} : fm{1E+10} : fm{2.5E-12}
NPrint ""
fm{1E+15} : fm{9E+17} : fm{1.25E-07} : fm{-7.5E-10} : fm{3E-16}
NPrint ""
FloatMode 0
NPrint "fm0"
fm{1E+07} : fm{1E+08} : fm{1E-07} : fm{1E-08} : fm{1.1E-08} : fm{9.9E-09} : fm{-1E-09}
NPrint ""
FloatMode 2 : fm{1.5} : FloatMode -5 : fm{1E+10} : FloatMode 0 : NPrint ""
; ---- mixed Print / NPrint
a$="text"
Print "a",1,"b",b,w,l
Print q,f,a$,"",-7
NPrint ""
NPrint 1,2,3
NPrint "str$ ",Str$(b)," ",Str$(w)," ",Str$(l)," ",Str$(q)," ",Str$(f)
NPrint "ustr$ ",UStr$(b)," ",UStr$(w)," ",UStr$(l)," ",UStr$(q)," ",UStr$(f)
s$=Str$(12)+Str$(-3.5)+Str$(1E+10) : NPrint "concat [",s$,"] ",Len(s$)
; ---- Format
Statement fmt{fs$}
  Format fs$
  Print "fmt '",fs$,"': "
  f.f=3.14159 : Print "[",f,"]"
  f=-2.5 : Print "[",f,"]"
  f=1234.5 : Print "[",f,"]"
  f=0.567 : Print "[",f,"]"
  f=999.96 : Print "[",f,"]"
  f=0 : Print "[",f,"]"
  l.l=42 : Print "[",l,"]"
  l=-7 : Print "[",l,"]"
  l=1234567 : Print "[",l,"]"
  q.q=12.75 : Print "[",q,"]"
  w.w=-300 : Print "[",w,"]"
  b.b=5 : Print "[",b,"]"
  NPrint "[",Str$(f),"|",UStr$(3.14159),"]"
  Format ""
End Statement
fmt{"###.##"} : fmt{"000.000"} : fmt{"#####"} : fmt{"##"} : fmt{"-###.#"} : fmt{"+###.#"}
fmt{"###.#-"} : fmt{"#,###.##"} : fmt{"#,###,###"} : fmt{",##"} : fmt{"$###.##"} : fmt{".##"}
fmt{"###.#"} : fmt{"0.0"} : fmt{"#"} : fmt{"-"} : fmt{"x"} : fmt{"##.##E"} : fmt{"0#0.#0#"}
fmt{"##############################.#"} : fmt{"(+##0.00)"} : fmt{"#.#.#"} : fmt{"0000000000"}
Format "##.##" : NPrint "after fmt [",1.5,"] [",Str$(2.5),"] [",UStr$(2.5),"]" : Format ""
NPrint "no fmt [",1.5,"]"
; (literal exponents only up to 18: smaller numbers with pf)
; ---- FloatMode -1 with a tiny number: the digits run into the table (BUG(orig))
FloatMode -1 : pf{$80000080} : pf{$C0000002} : NPrint "after [",123.5,"]" : FloatMode 0
NPrint "small [",1E+17,"] [",-42.125,"]"
; ---- Format longer than 32 characters: runtime error with the debugger,
; without it the copy runs over formlen (here 3) and formlead
Format String$("#",32)+Chr$(0)+Chr$(3)+"##"
NPrint "long fmt [",7,"] [",-12.5,"]"
Format ""
NPrint "end"
End

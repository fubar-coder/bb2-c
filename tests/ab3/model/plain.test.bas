; plain - the run-output cases of AB3 mode without a compiler state of
; their own (the default OPTIMIZE and SYNTAX, no header), in one file so
; that AmiBlitz3 compiles them in one session. Each case was a case of
; tests/ab3/CMakeLists.txt and has its name. A value a procedure printed
; is passed in zo$ (a string) or zn (a Long).
!TestBatchInit
  zo$="" : zn.l=0
!EndTestBatchInit
; funcstmt_num: a Function called as a statement: compiled as a
; Statement's call, the result discarded (AmiBlitz3.ab3:4416-4426, ;BPL.w
; illprocerr)
Function.l myadd{a.l,b.l}
  SHARED zn
  zn=a + b
  Function Return a + b
End Function
; funcstmt_str: a string Function called as a statement: its result is
; left in the string buffer, the strings of the program stay intact
Function.s mygreet{n$}
  SHARED zo$
  zo$="hello "+n$
  Function Return "<" + n$ + ">"
End Function
; funcstmt_nopar: a Function without parameters called as a statement,
; also one that ends without Function Return
Function.w mycount{}
  SHARED n.w
  n = n + 1
  Function Return n
End Function
Function.l mynothing{}
End Function
!TestCase{plus}
  ; the + prefix of a statement (no type check): compiled as without it
  pw.w = 3
+a.l = pw.w
  !CheckEq{a,3,"l1"}
!EndTestCase
!TestCase{funcstmt_num}
  myadd{2,3}
  !CheckEq{zn,5,"l1"}
  x.l = myadd{4,5}
  !CheckEq{zn,9,"l2"}
  zs$="sum "+Str$(x)
  !CheckStrEq{zs$,"sum 9","l3"}
!EndTestCase
!TestCase{funcstmt_str}
  a$ = "abc"
  mygreet{"world"}
  !CheckStrEq{zo$,"hello world","l1"}
  !CheckStrEq{"next","next","l2"}
  b$ = mygreet{a$}
  !CheckStrEq{zo$,"hello abc","l3"}
  zs$=a$+"x"+b$
  !CheckStrEq{zs$,"abcx<abc>","l4"}
  mygreet{a$ + "def"}
  !CheckStrEq{zo$,"hello abcdef","l5"}
  !CheckStrEq{a$,"abc","l6"}
!EndTestCase
!TestCase{funcstmt_nopar}
  mycount{} : mycount{} : mynothing{} : zs$="count "+Str$(mycount{})
  !CheckStrEq{zs$,"count 3","l1"}
!EndTestCase
; macthen_while: a one-line If in a macro ends with its macro line
; (AmiBlitz3 appends ": End If"): macro mtw_a inside the While loop of
; macro mtw_b
Macro mtw_a
If `1 > 0 Then `1 = 0
End Macro
Macro mtw_b
While `1 > 0
  !mtw_a{`1}
Wend
End Macro
; macthen_after: the line after a one-line If in a macro is not
; conditional
Macro mta_a
If `1 > 0 Then `1 = 0
`2 = `2 + 10
End Macro
; macthen_closed: a macro line already closed with End If gets no second
; one
Macro mtc_a
If `1 > 0 Then `1 = 0 : End If
`2 = `2 + 10
End Macro
; macthen_join: lines joined with @@ are one macro line: the End If goes
; behind the last of them, both assignments are conditional
Macro mtj_a
If `1 > 0 Then `1 = 0 : @@
`2 = 0
End Macro
; macthen_caller: a one-line If of the caller covers the whole macro and
; the rest of the calling line, also with a one-line If inside the macro
Macro mtl_m
zs$=zs$+"m1"
zs$=zs$+"m2"
zs$=zs$+"m3"
End Macro
Macro mtl_r
If `1 > 0 Then `1 - 1
`2 + 100
End Macro
; macthen_else: Then and Else on one macro line: one End If
Macro mte_e
If `1 > 0 Then `2 = 1 Else `2 = 2
`3 = 1
End Macro
; macthen_string: Then in a string and in a comment of a macro line does
; not count
Macro mts_s
zs$="If Then" ; Then
`1 = 1
End Macro
; macthen_endif: the End If of a block before a one-line If on the same
; macro line does not close it; a block If around a one-liner in a macro
Macro mtf_f
If `1 > 0
`2 = 1
End If : If `2 > 0 Then `2 = 2
`3 = 1
End Macro
Macro mtf_g
If `1 > 0
If `2 > 0 Then `2 = 0
Else
`2 = 9
End If
End Macro
; macend_if: a macro line ending in End inside a block If: the rest of the
; macro is stored (Blitz Basic 2 loses it, #968)
Macro m
If `1
End
End If
zs$="after"
End Macro
!TestCase{macthen_while}
  x.l = 1 : !mtw_b{x}
  !CheckEq{x,0,"l1"}
!EndTestCase
!TestCase{macthen_after}
  x = 0 : y.l = 5 : !mta_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 15","l1"}
  x = 1 : y = 5 : !mta_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 15","l2"}
!EndTestCase
!TestCase{macthen_closed}
  x = 1 : y = 5 : !mtc_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 15","l1"}
  x = 0 : y = 5 : !mtc_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 15","l2"}
!EndTestCase
!TestCase{macthen_join}
  x = 0 : y = 5 : !mtj_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 5","l1"}
  x = 1 : y = 5 : !mtj_a{x, y} : zs$=Str$(x)+" "+Str$(y)
  !CheckStrEq{zs$,"0 0","l2"}
!EndTestCase
!TestCase{macthen_caller}
  c.l = 0 : zs$="" : If c Then !mtl_m{}
  !CheckStrEq{zs$+"after","after","l1"}
  c = 0 : e.l = 0 : If e Then !mtl_r{c, e} : e = 7
  !CheckStrEq{Str$(c)+" "+Str$(e),"0 0","l2"}
  c = 0 : e = 1 : If e Then !mtl_r{c, e} : e = 7
  !CheckStrEq{Str$(c)+" "+Str$(e),"0 7","l3"}
!EndTestCase
!TestCase{macthen_else}
  a.l = 0 : b.l = 0 : c = 0 : !mte_e{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"0 2 1","l1"}
  a = 1 : b = 0 : c = 0 : !mte_e{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"1 1 1","l2"}
!EndTestCase
!TestCase{macthen_string}
  x = 0 : !mts_s{x}
  !CheckStrEq{zs$,"If Then","l1"}
  !CheckEq{x,1,"l2"}
!EndTestCase
!TestCase{macthen_endif}
  a = 0 : b = 0 : c = 0 : !mtf_f{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"0 0 1","l1"}
  a = 1 : b = 0 : c = 0 : !mtf_f{a, b, c}
  !CheckStrEq{Str$(a)+" "+Str$(b)+" "+Str$(c),"1 2 1","l2"}
  a = 1 : b = 5 : !mtf_g{a, b}
  !CheckEq{b,0,"l3"}
  a = 0 : b = 5 : !mtf_g{a, b}
  !CheckEq{b,9,"l4"}
!EndTestCase
!TestCase{macend_if}
  zs$="" : !m{0}
  !CheckStrEq{zs$,"after","l1"}
  !CheckStrEq{"done","done","l2"}
!EndTestCase
; tilde_string: a '~' inside a string literal is an ordinary character
; (#966): in Dc.b, in string literals (also one that holds a valid ~expr~),
; in a Data$ string; nothing is computed inside strings. The literal with
; '~' is the actual value, the expected one is built with Chr$(126). The data
; stands behind the case: AmiBlitz3's Restore of a label above it reads nothing.
!TestCase{tilde_string}
  !CheckStrEq{Peek$(?tsa1),"J"+Chr$(126),"l1"}
  !CheckStrEq{"x~y","x"+Chr$(126)+"y","l2"}
  !CheckStrEq{"a~1+2~b","a"+Chr$(126)+"1+2"+Chr$(126)+"b","l3"}
  Restore tsd1 : Read s$
  !CheckStrEq{s$,"d"+Chr$(126)+"1","l4"}
!EndTestCase
.tsa1
Dc.b "J~",0
Even
.tsd1
Data$ "d~1"
; byte_fraction_const: a constant with a fraction under a Byte type so far
; (#1188): it keeps its fraction, the operation is computed in Float and
; the result converted to the Byte at the end, as under a Word type so
; far; the same in AmiBlitz3 v3.14.0 (the whole-valued literals:
; byte-fraction-whole.test.bas)
NEWTYPE.nb
  f.b
End NEWTYPE
Statement sb{x.b}
  SHARED zn
  zn=x
End Statement
Function.b fb{x.b}
  Function Return x
End Function
; intrinsic_frac: Frac of a Quick assigned to a Quick in AB3 mode (change
; compiler-intrinsics, spec compiler/intrinsics): the fraction with the
; sign of the value. AB3 mode's vallib (AmiBlitz3's) takes a Float for
; Frac, which is not the form of the intrinsic: the call stays ("calls 3":
; qutofl, Frac, fltoqu), as in AmiBlitz3
Function.l jsrs{a.l,b.l}
  n.l = 0
  While a < b
    If Peek.w(a) = $4EB9
      n = n + 1 : a = a + 4
    EndIf
    a = a + 2
  Wend
  Function Return n
End Function
!TestCase{byte_fraction_const}
  vb.b=2 : rb.b=0
  Dim ab.b(2) : nv.nb\f=0
  rb=(1.5+1.5)
  !CheckEq{rb,3,"l1"}
  rb=(0.5*4)
  !CheckEq{rb,2,"l2"}
  rb=(1.5*vb)
  !CheckEq{rb,3,"l3"}
  rb=(2.5*2)
  !CheckEq{rb,5,"l4"}
  rb=(-1.5*vb)
  !CheckEq{rb,-3,"l5"}
  rb=(1.25e1*2)
  !CheckEq{rb,25,"l6"}
  rb=1.5
  !CheckEq{rb,1,"l7"}
  rb=-1.5
  !CheckEq{rb,-1,"l8"}
  sb{1.5*vb}
  !CheckEq{zn,3,"l9"}
  rb=fb{1.5*vb}
  !CheckEq{rb,3,"l10"}
  nv\f=(1.5+1.5)
  !CheckEq{nv\f,3,"l11"}
  ab(1)=(1.5*vb)
  !CheckEq{ab(1),3,"l12"}
!EndTestCase
!TestCase{intrinsic_frac}
  q1.q = 0 : q2.q = 0
  q1 = 1.25 : q2 = Frac(q1)
  !CheckQEq{q2,0.25,"l1"}
  q1 = -1.25 : q2 = Frac(q1)
  !CheckQEq{q2,-0.25,"l2"}
  q1 = 0.5 : q2 = Frac(q1)
  !CheckQEq{q2,0.5,"l3"}
  q1 = -3 : q2 = Frac(q1)
  !CheckQEq{q2,0,"l4"}
  q1 = 300.75 : q2 = Frac(q1)
  !CheckQEq{q2,0.75,"l5"}
  q1 = -1.25
.a1
  q2 = Frac(q1)
.a2
  k.l=jsrs{?a1, ?a2} : zs$=Str$(q2)+" calls "+Str$(k)
  !CheckStrEq{zs$,"-.25 calls 3","l6"}
!EndTestCase
!TestCase{opt_cnif}
  ; OPTIMIZE inside a skipped CNIF block has no effect: the FPU bit there
  ; is no error
  CNIF 1 = 0
  OPTIMIZE 7
  CEND
  !CheckStrEq{"compiled","compiled","l1"}
!EndTestCase
!TestCase{libtoken_past_block}
  ; a library whose entries run past its block of 127 numbers while the
  ; next library number is free (#960): Mildred (library 15, there is no
  ; library 16) has MProcessor as its entry 206, the token of library 16's
  ; entry 78; the compiler finds it in Mildred. The CPU number is not
  ; checked, only that the function gives one of 0..6 (MC68000..MC68060)
  p.w = MProcessor : If p >= 0 AND p <= 6 Then zs$="cpu ok" Else zs$="cpu "+Str$(p)
  !CheckStrEq{zs$,"cpu ok","l1"}
!EndTestCase
!TestCase{asm_short_s}
  ; the size suffix .s of an assembler instruction (AmiBlitz3 v3.14.0: .s
  ; is .b; a branch with .s stays a word branch): BEQ.s is taken, and the
  ; BRA.s with its target is 4 bytes
  MOVEQ #5,d0
  CMP.l #5,d0
  BEQ.s ok
  MOVEQ #0,d0
ok:
  a.l = PutD0
  !CheckEq{a,5,"l1"}
l1:
  BRA.s l2
l2:
  !CheckEq{?l2 - ?l1,4,"l2"}
!EndTestCase

; expr-libcalls - library functions in an expression: libraries used
; first in another library function's parameter (the outer library is
; used at its lookup, before its parameters), functions without
; parameters and with each of their forms (Rnd, Mid$, Instr), with a
; size specifier (Peek.b, .w, .l and none), parameters of unknown type
; (Str$, Abs, Sgn) of each type, a string result in a string expression
; and a numeric one in a string function's parameter, calls as the right
; operand at regat 5 and 6 (the registers below saved around them), a
; function that loads a3 in a string expression (#46), forms with
; repeats (GTSetAttrs: one and two repeats, the parameters on the
; stack), and the intrinsics Int and Frac (a Quick, a Long and a string
; argument, as the whole right side and inside an expression) and Len
; and Asc (a plain string variable, one without memory, an empty one and
; an expression).
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-3d-library-calls): their executable
; is compared with the previous commit's (blitz2_expr_libcalls_prev),
; the values here are their results.  The lines behind "If never" are
; compiled and not run: the function needs an object that is not there
; (a GTList, a Sound) or refuses its argument at run time (Int and Frac
; of a string).
; Correct: every case gives the values of its checks.
DEFTYPE .l

!TestCase{first_use}
  ; each library used first here: the outer one at its lookup, the inner
  ; one in the outer's parameter
  w.w=$1234
  a$=Hex$(Peek.w(&w)) : !CheckStrEq{a$,"00001234","peek_in_hex"}
  f.f=Sqr(Abs(-16)) : !CheckFltNear{f,4,0.0001,"abs_in_sqr"}
  d=NumDays(Date$(100)) : !CheckEq{d,100,"date_in_numdays"}
!EndTestCase

!TestCase{forms}
  q.q=Rnd : !Check{q>=0 AND q<1}
  q=Rnd(10) : !Check{q>=0 AND q<10}
  !CheckStrEq{Mid$("abcdef",2,3),"bcd","mid3"}
  !CheckStrEq{Mid$("abcdef",4),"def","mid2"}
  !CheckEq{Instr("abcabc","c"),3,"instr2"}
  !CheckEq{Instr("abcabc","c",4),6,"instr3"}
  b.b=-5 : w.w=-300 : l.l=70000
  b2.b=Peek.b(&b) : !CheckEq{b2,-5,"peek_b"}
  w2.w=Peek.w(&w) : !CheckEq{w2,-300,"peek_w"}
  l2.l=Peek.l(&l) : !CheckEq{l2,70000,"peek_l"}
  l2=Peek(&l) : !CheckEq{l2,70000,"peek_default"}
  l2=1+Peek.w(&w)*2 : !CheckEq{l2,-599,"peek_in_an_expression"}
  q=Peek.b(&b)/2 : !CheckQEq{q,-2.5,"byte_under_quick"}
!EndTestCase

!TestCase{unknown}
  b.b=-5 : w.w=300 : l.l=70000 : q.q=1.5 : f.f=2.5
  !CheckStrEq{Str$(b),"-5","byte"}
  !CheckStrEq{Str$(w),"300","word"}
  !CheckStrEq{Str$(l),"70000","long"}
  !CheckStrEq{Str$(q),"1.5","quick"}
  !CheckStrEq{Str$(f),"2.5","float"}
  !CheckStrEq{Str$(l*2+w),"140300","expression"}
  f=Abs(b) : !CheckFltNear{f,5,0.0001,"abs_byte"}
  f=Abs(-q) : !CheckFltNear{f,1.5,0.0001,"abs_quick"}
  q=Sgn(-l) : !CheckQEq{q,-1,"sgn_long"}
  !CheckStrEq{Str$(Abs(-f)),"1.5","nested"}
!EndTestCase

!TestCase{strings}
  a$="<"+UCase$("ab")+">" : !CheckStrEq{a$,"<AB>","string_result"}
  a$="<"+Str$(Len("abc"))+">" : !CheckStrEq{a$,"<3>","len_in_str"}
  a$=Left$("abc",Len("ab")) : !CheckStrEq{a$,"ab","len_in_left"}
  a$="["+Mid$(UCase$("xyz"),Instr("xyz","y"))+"]"
  !CheckStrEq{a$,"[YZ]","nested"}
!EndTestCase

!TestCase{regat}
  a$="abc" : l=4
  r=1+(2+(3+(4+(5+Len(a$+"d"))))) : !CheckEq{r,19,"regat5"}
  r=1+(2+(3+(4+(5+(6+Instr(a$,"c")))))) : !CheckEq{r,24,"regat6"}
  r=1+(2+(3+(4+(5+(6+Peek.l(&l)))))) : !CheckEq{r,25,"regat6_size"}
  a$="a"+(Str$(1+(2+(3+(4+(5+Peek.l(&l)))))))
  !CheckStrEq{a$,"a19","regat5_string"}
!EndTestCase

!TestCase{compiled}
  never=0
  ; #46: PeekSound loads a3 (its Sound) in a string expression
  If never=1 Then a$="<"+Str$(PeekSound(0,1))+">"
  ; repeats: one and two (Tag,Value) pairs pushed
  If never=1 Then r=GTSetAttrs(0,1,2,3)
  If never=1 Then r=GTSetAttrs(0,1,2,3,4,5)
  If never=1 Then r=1+(2+GTSetAttrs(0,1,2,3,4,5))
  ; the intrinsics' string argument
  If never=1 Then l=Int("3")
  If never=1 Then q.q=Frac("3")
  !CheckEq{never,0,"not_run"}
!EndTestCase

!TestCase{intrinsics}
  q.q=-1.75 : q2.q=-1.25 : l.l=7 : f.f=2.5
  l2.l=Int(q) : !CheckEq{l2,-1,"int_quick"}
  l2=Int(q)+10 : !CheckEq{l2,9,"int_quick_expression"}
  l2=Int(l) : !CheckEq{l2,7,"int_long"}
  l2=Int(l)*2 : !CheckEq{l2,14,"int_long_expression"}
  l2=Int(f) : !CheckEq{l2,2,"int_float"}
  l2=Int(q+Int(q)) : !CheckEq{l2,-2,"int_nested"}
  q=Frac(q2) : !CheckQEq{q,-0.25,"frac_quick"}
  q=Frac(q2) : l2=1 : !CheckQEq{q,-0.25,"frac_quick_colon"}
  q=Frac(q2)+1 : !CheckQEq{q,0.75,"frac_quick_expression"}
  f=Frac(q2) : !CheckFltNear{f,-0.25,0.0001,"frac_quick_float"}
  q=Frac(l) : !CheckQEq{q,0,"frac_long"}
  q=Frac(l)+0.5 : !CheckQEq{q,0.5,"frac_long_expression"}
  q=Frac(Frac(q2)+2) : !CheckQEq{q,0.75,"frac_nested"}
  a$="hello" : e$=""
  l2=Len(a$) : !CheckEq{l2,5,"len_var"}
  l2=Len( a$ ) : !CheckEq{l2,5,"len_var_spaces"}
  l2=Len(a$)+1 : !CheckEq{l2,6,"len_var_expression"}
  l2=Len(e$) : !CheckEq{l2,0,"len_empty"}
  l2=Len(n$) : !CheckEq{l2,0,"len_no_memory"}
  l2=Len(a$+"!") : !CheckEq{l2,6,"len_expression"}
  l2=Len(Str$(Int(q2))) : !CheckEq{l2,2,"len_nested"}
  w.w=Asc(a$) : !CheckEq{w,104,"asc_var"}
  w=Asc(a$)-Asc("a") : !CheckEq{w,7,"asc_var_expression"}
  w=Asc(e$) : !CheckEq{w,-1,"asc_empty"}
  w=Asc(m$) : !CheckEq{w,-1,"asc_no_memory"}
  w=Asc(Mid$(a$,2)) : !CheckEq{w,101,"asc_expression"}
!EndTestCase

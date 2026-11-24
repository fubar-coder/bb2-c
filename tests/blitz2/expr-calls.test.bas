; expr-calls - procedure calls (Function f{...}) and OS calls (Name_(...))
; in an expression: without parameters and with them, each result type
; under each kind of type so far (a Byte extended, a value converted up,
; the type so far raised), a call nested in its own parameter, a
; recursion, a call in a string expression (a string result and one that
; is not), calls as the right operand at regat 5 and 6 (the registers
; d0-d4 or d0-d5 saved around them), OS calls without brackets, with
; empty ones and with parameters, twice in a row, after a library call
; and with parameters in data registers that hold a value of the
; expression (#344).
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-3c-calls): their executable is
; compared with the previous commit's (blitz2_expr_calls_prev), the values
; here are their results.  A Function used before its definition is
; "Procedure not found" (no forward reference: the lookup finds only the
; procedures defined so far).  An OS call after a Float operation in the
; same expression whose result goes into a Float operation is an open
; original bug (compiler-amigalib-lasta6), so the OS calls here are not
; in Float expressions.
; Correct: every case gives the values of its checks.
DEFTYPE .l
Function.b fb{x.l}
  Function Return x
End Function
Function.w fw{x.l}
  Function Return x
End Function
Function.l fl{x.l}
  Function Return x*2
End Function
Function.q fq{x.q}
  Function Return x/2
End Function
Function.f ff{x.f}
  Function Return x*1.5
End Function
Function$ fs{a$}
  Function Return "["+a$+"]"
End Function
Function.l f0{}
  Function Return 7
End Function
Function.l f2{a.l,b.l}
  Function Return a*10+b
End Function
Function.l f6{a.l,b.l,c.l,d.l,e.l,f.l}
  Function Return a+b+c+d+e+f
End Function
Function.l fact{n.l}
  If n<2 Then Function Return 1
  Function Return n*fact{n-1}
End Function

!TestCase{results}
  !CheckEq{f0{},7,"noparams"}
  !CheckEq{f0{}+f0{},14,"noparams_twice"}
  !CheckEq{fl{3},6,"long"}
  !CheckEq{f2{1,2},12,"two"}
  !CheckEq{f6{1,2,3,4,5,6},21,"six"}
  w.w=fb{-3}+1 : !CheckEq{w,-2,"byte_under_word"}
  b.b=fb{-3} : !CheckEq{b,-3,"byte_under_byte"}
  l.l=fw{-300}*2 : !CheckEq{l,-600,"word_under_long"}
  q.q=fl{2}+0.5 : !CheckQEq{q,4.5,"long_under_quick"}
  q=fq{3} : !CheckQEq{q,1.5,"quick"}
  l=fq{3}*2 : !CheckEq{l,3,"quick_under_long"}
  f.f=fq{3}+ff{2} : !CheckFltNear{f,4.5,0.0001,"quick_float"}
  f=fl{3}/4 : !CheckFltNear{f,1.5,0.0001,"long_under_float"}
  f=ff{2} : !CheckFltNear{f,3,0.0001,"float"}
  !CheckStrEq{fs{"a"},"[a]","string"}
!EndTestCase

!TestCase{nested}
  !CheckEq{fl{fl{fl{1}}},8,"own_parameter"}
  !CheckEq{f2{fl{1},f2{fl{2},3}},2*10+43,"parameters"}
  !CheckEq{fl{1+fl{2}*3},26,"in_an_expression"}
  !CheckEq{fact{5},120,"recursion"}
  q.q=fq{fq{fq{4}}} : !CheckQEq{q,0.5,"quick"}
  !CheckEq{f6{1,2,3,4,5,f6{1,1,1,1,1,fl{1}}},22,"six"}
!EndTestCase

!TestCase{strings}
  a$="<"+fs{"x"}+">" : !CheckStrEq{a$,"<[x]>","string_result"}
  a$="<"+Str$(fl{3})+">" : !CheckStrEq{a$,"<6>","long_result"}
  a$=fs{fs{"y"}}+fs{"z"} : !CheckStrEq{a$,"[[y]][z]","nested"}
  a$="<"+fs{Str$(f2{1,2})}+">" : !CheckStrEq{a$,"<[12]>","parameter"}
!EndTestCase

!TestCase{regat}
  x=3
  ; the call the right operand at regat 5 and 6: d0-d4, d0-d5 saved
  r=1+(2+(3+(4+(5+fl{x})))) : !CheckEq{r,21,"regat5"}
  r=1+(2+(3+(4+(5+(6+fl{x}))))) : !CheckEq{r,27,"regat6"}
  r=1+(2+(3+(4+(5+(6+f2{x,fl{x}}))))) : !CheckEq{r,57,"regat6_params"}
  r=1+(2+(3+(4+(5+(6+f0{}))))) : !CheckEq{r,28,"regat6_noparams"}
  a$="a"+(Str$(1+(2+(3+(4+(5+fl{x}))))))
  !CheckStrEq{a$,"a21","regat5_string"}
!EndTestCase

!TestCase{os}
  t=FindTask_(0)
  !CheckEq{FindTask_(0)-t,0,"one"}
  !CheckEq{FindTask_(0)+FindTask_(0)-t-t,0,"twice"}
  o=Output_
  !CheckEq{Output_-o,0,"no_brackets"}
  !CheckEq{Output_()-o,0,"empty_brackets"}
  !CheckEq{Asc("A")+FindTask_(0)-t,65,"after_a_library_call"}
  !CheckEq{FindTask_(0)-t+Asc("A"),65,"before_a_library_call"}
  m=AllocMem_(16,1)
  !CheckEq{TypeOfMem_(m) AND 1,1,"memory"}
  FreeMem_ m,16
  ; #344: d1 and d2, two of the parameter registers, hold values of the
  ; expression (saved around the call)
  r=1+(2+(3+Write_(o,&x,0))) : !CheckEq{r,6,"param_regs"}
  r=1+(2+(3+(4+(5+(Output_-o))))) : !CheckEq{r,15,"regat5"}
  r=1+(2+(3+(4+(5+(6+(Output_-o)))))) : !CheckEq{r,21,"regat6"}
  !CheckEq{fl{FindTask_(0)-t+2},4,"in_a_parameter"}
  !CheckEq{FindTask_(fl{0})-t,0,"a_call_in_its_parameter"}
!EndTestCase

; expr-lookahead - the forms whose code the compiler decides from what it
; has read beyond the element or written before it: a later array index
; narrowed to its low word (arrevalchk: a constant as move.w, the ext.l
; of one Byte or Word element left out; not for a sum, a fold, a Long,
; a negated value or with runtime error checking), on the right side and
; on the left side of an assignment and in Select; library functions
; whose form is chosen by the number of their parameters (countpees),
; nested eight deep with commas and brackets inside string literals and
; brackets; the intrinsics (Int, Frac of a Quick, Len and Asc of a string
; variable) as the whole right side and as a part of it; assignments
; whose left side has code (a global NewType array, a string array, a
; path with \, an index with a call), whose code the compiler writes
; behind the right side's; Select on an array element (a string one gets
; its clr before the expression's code).
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-4b-lookaheads): their executable is
; compared with the previous commit's (blitz2_expr_lookahead_prev), the
; values here are their results.  A left side whose index calls a library
; function with its base in a6 and a right side that calls one with the
; same base is an open original bug (compiler-let-left-a6), so no case
; here combines them.
; Correct: every case gives the values of its checks.
DEFTYPE .l
NEWTYPE .mt
  x.w
  y.l
  s$
End NEWTYPE
Function.w fw{x.l}
  Function Return x
End Function
Function.l fl{x.l,y.l}
  Function Return x*10+y
End Function

!TestBatchInit
  Dim a.l(10,10), b.w(4,4,4), g.mt(5), s$(5)
!EndTestBatchInit

!TestCase{index_narrow}
  i.l = 2 : j.w = 3 : bt.b = 4 : q.q = 1.5 : l.l = 5
  a(2,3) = 23 : a(2,4) = 24 : a(2,5) = 25 : a(2,6) = 26 : a(2,1) = 21
  !CheckEq{a(i,3),23,"const"}
  !CheckEq{a(i,j),23,"word"}
  !CheckEq{a(i,bt),24,"byte"}
  !CheckEq{a(i,(j)),23,"bracket"}
  !CheckEq{a(i,fw{j}),23,"call"}
  !CheckEq{a(i,q*3),24,"raw_quick"}
  !CheckEq{a(i,q),21,"quick"}
  #c = 5
  !CheckEq{a(i,#c),25,"named_const"}
  b(1,2,3) = 123 : bi.w = 1 : bj.w = 2 : bk.w = 3
  !CheckEq{b(bi,bj,bk),123,"three"}
  !CheckEq{b(1,2,3),123,"three_const"}
!EndTestCase

!TestCase{index_wide}
  i.l = 2 : j.w = 3 : l.l = 5
  a(2,3) = 23 : a(2,4) = 24 : a(2,5) = 25 : a(2,7) = 27
  !CheckEq{a(i,j+1),24,"sum"}
  !CheckEq{a(i,l),25,"long"}
  !CheckEq{a(i,2+3),25,"fold"}
  !CheckEq{a(i,-(-j)),23,"neg"}
  !CheckEq{a(i,fl{0,7}),27,"long_call"}
  !CheckEq{a(i,a(i,j)-20),23,"nested"}
!EndTestCase

!TestCase{index_left}
  i.l = 3 : j.w = 4 : bt.b = 5 : l.l = 6
  a(i,j) = 34 : a(i,bt) = 35 : a(i,l) = 36 : a(i,2) = 32 : a(i,j+3) = 37
  !CheckEq{a(3,4),34,"word"}
  !CheckEq{a(3,5),35,"byte"}
  !CheckEq{a(3,6),36,"long"}
  !CheckEq{a(3,2),32,"const"}
  !CheckEq{a(3,7),37,"sum"}
!EndTestCase

!TestCase{params}
  t$ = "ab,cd(e)f,gh"
  u$ = Mid$(Mid$(Mid$(Mid$(Mid$(Mid$(Mid$(Mid$(t$,1),1,12),1),1,12),1),1,12),1),2,3)
  !CheckStrEq{u$,"b,c","mid8"}
  n.l = Instr(Left$(Mid$(t$+",(x)",Instr("q,(r)",",")),Len(Mid$("a,b",1,2))+9),",",Instr(t$,"(")-2)
  !CheckEq{n,9,"instr"}
  n = fl{Instr("(,)",","),fl{Len(Mid$("1,2)",2)),(1+(2))}}
  !CheckEq{n,53,"proc"}
  u$ = Mid$("x(y,z)w",Instr("a,b",","),Len("(,"))
  !CheckStrEq{u$,"(y","mid_count"}
!EndTestCase

!TestCase{intrinsics}
  q.q = 2.75 : r.q = 0 : l.l = 0 : f.f = 0 : a$ = "hello" : e$ = ""
  l = Int(q)
  !CheckEq{l,2,"int_whole"}
  l = Int(q)+1
  !CheckEq{l,3,"int_part"}
  r = Frac(q)
  !CheckQEq{r,0.75,"frac_whole"}
  r = Frac(q) : l = 1
  !CheckQEq{r,0.75,"frac_colon"}
  r = Frac(q) ; a comment behind it
  !CheckQEq{r,0.75,"frac_comment"}
  r = Frac(q)+0.25
  !CheckQEq{r,1,"frac_part"}
  r = 0.25+Frac(q)
  !CheckQEq{r,1,"frac_right"}
  f = Frac(q)
  !CheckFltNear{f,0.75,0.001,"frac_float"}
  l = Len(a$)
  !CheckEq{l,5,"len_whole"}
  l = Len( a$ )+1
  !CheckEq{l,6,"len_part"}
  l = Len(a$+"x")
  !CheckEq{l,6,"len_expr"}
  l = Len(e$)
  !CheckEq{l,0,"len_empty"}
  l = Asc(a$)
  !CheckEq{l,104,"asc_whole"}
  l = Asc(a$)+Len(a$)
  !CheckEq{l,109,"asc_part"}
  l = Asc(e$)
  !CheckEq{l,-1,"asc_empty"}
!EndTestCase

!TestCase{let_left}
  i.l = 2 : j.w = 3 : v.mt\x = 0
  g(i)\x = 7 : g(i)\y = j*100 : g(i)\s = "gs"
  !CheckEq{g(2)\x,7,"newtype_w"}
  !CheckEq{g(2)\y,300,"newtype_l"}
  !CheckStrEq{g(2)\s,"gs","newtype_s"}
  s$(j) = "s"+Mid$("x2",i) : s$(fw{i}) = "t"
  !CheckStrEq{s$(3),"s2","string_array"}
  !CheckStrEq{s$(2),"t","string_call"}
  v\x = 9 : v\y = v\x*2 : v\s = "vs"+s$(j)
  !CheckEq{v\y,18,"path"}
  !CheckStrEq{v\s,"vss2","path_s"}
  a(fw{i},Abs(-j)) = j+2
  !CheckEq{a(2,3),5,"call_left"}
  a(fw{i},3) = Abs(-6)
  !CheckEq{a(2,3),6,"call_both"}
  g(i+1)\x = 4,5,"cs"
  !CheckEq{g(3)\x+g(3)\y,9,"comma"}
  !CheckStrEq{g(3)\s,"cs","comma_s"}
!EndTestCase

!TestCase{select_elem}
  i.l = 1 : j.w = 2 : rr.l = 0
  a(1,2) = 12 : s$(1) = "two"
  Select a(i,j)
    Case 11 : rr = 1
    Case 12 : rr = 2
    Default : rr = 3
  End Select
  !CheckEq{rr,2,"array"}
  Select s$(i)
    Case "one" : rr = 4
    Case "two" : rr = 5
  End Select
  !CheckEq{rr,5,"string_array"}
  Select g(i)\x+fw{j}
    Case 2 : rr = 6
    Default : rr = 7
  End Select
  !CheckEq{rr,6,"newtype"}
!EndTestCase

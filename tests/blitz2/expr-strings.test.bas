; expr-strings - string elements and the operators on strings: literals
; (empty, odd and even length: the pad byte of the literal area), string
; variables (simple ones, NewType fields and array elements: their address
; in a2), string results of Functions and library functions, long chains
; of + with literals and functions, + nested to d6 and beyond (the operand
; kept on the stack), the six comparisons with a variable, a literal, a
; function and an expression on either side, comparisons as operands of
; AND, OR, NOT and of arithmetic, strings in the parameters of Functions
; (nested, two of them, a string expression in a numeric Function's
; parameter), and a second string expression in one statement (the work
; area's a3 saved and restored: makesbase, sbtolong).
; The cases pin the code of these forms for the restructuring of the
; evaluator (#1180, change expr-tree-3e-strings): their executable is
; compared with the previous commit's (blitz2_expr_strings_prev), the
; values here are their results.  The lines behind "If never" are compiled
; and not run: NPrint takes its arguments with their type (a string
; length behind the pushed type word), a string under a numeric type so
; far (its address).
; Correct: every case gives the values of its checks.
DEFTYPE .l
NEWTYPE .st
  n.w
  s$
End NEWTYPE
Function$ fs{a$}
  Function Return "["+a$+"]"
End Function
Function$ f2s{a$,b$}
  Function Return a$+"/"+b$
End Function
Function.l fn{a$}
  Function Return Len(a$)*2
End Function
Function.l fsn{a$,n.l}
  Function Return Len(a$)+n
End Function

!TestCase{literals}
  a$="" : !CheckEq{Len(a$),0,"empty"}
  a$="a" : !CheckStrEq{a$,"a","odd"}
  a$="ab" : !CheckStrEq{a$,"ab","even"}
  a$="abc"+"" : !CheckStrEq{a$,"abc","empty_right"}
  a$=""+"abc" : !CheckStrEq{a$,"abc","empty_left"}
  a$="" : b$="" : a$=a$+b$ : !CheckEq{Len(a$),0,"both_empty"}
  !CheckEq{Len("hello"),5,"len_literal"}
!EndTestCase

!TestCase{variables}
  Dim d$(3)
  t.st\s="field" : d$(2)="element"
  a$=t\s+"-"+d$(2) : !CheckStrEq{a$,"field-element","field_and_element"}
  a$=d$(2)+t\s : !CheckStrEq{a$,"elementfield","element_and_field"}
  i=2 : a$="<"+d$(i)+">" : !CheckStrEq{a$,"<element>","element_index"}
  !CheckEq{Len(t\s+d$(1)),5,"empty_element"}
  a$=n$+"x" : !CheckStrEq{a$,"x","no_memory"}
!EndTestCase

!TestCase{chains}
  a$="ab" : b$="cd"
  c$=a$+"-"+b$+"-"+Str$(12)+"-"+UCase$(a$)+"-"+Left$(b$,1)+"-"+Chr$(65)
  !CheckStrEq{c$,"ab-cd-12-AB-c-A","long_chain"}
  c$="1"+"2"+"3"+"4"+"5"+"6"+"7"+"8"+"9"+"10"+"11"+"12"+"13"+"14"+"15"+"16"
  !CheckStrEq{c$,"12345678910111213141516","literal_chain"}
  c$=a$+b$+a$+b$+a$+b$+a$+b$+a$+b$+a$+b$+a$+b$+a$+b$
  !CheckEq{Len(c$),32,"variable_chain"}
  c$=fs{a$}+fs{b$}+fs{a$+b$}+Mid$(c$,3,4)+Right$(a$,1)
  !CheckStrEq{c$,"[ab][cd][abcd]cdabb","function_chain"}
  c$="1"+("2"+("3"+("4"+("5"+("6"+"7")))))
  !CheckStrEq{c$,"1234567","nested_d6"}
  c$=a$+(b$+(a$+(b$+(a$+(b$+(a$+(b$+"!")))))))
  !CheckStrEq{c$,"abcdabcdabcdabcd!","nested_d7"}
  c$=(a$+b$)+(b$+a$) : !CheckStrEq{c$,"abcdcdab","brackets"}
  c$=a$+Str$(Len(a$+b$)+1)+b$ : !CheckStrEq{c$,"ab5cd","number_inside"}
!EndTestCase

!TestCase{compares}
  a$="ab" : b$="cd" : e$=""
  !Check{a$=a$} : !Check{a$<>b$} : !Check{a$<b$}
  !Check{a$<=b$} : !Check{b$>a$} : !Check{b$>=a$}
  !Check{a$="ab"} : !Check{"ab"=a$} : !Check{"cd"<>a$}
  !Check{a$+b$="abcd"} : !Check{"abcd"=a$+b$}
  !Check{UCase$(a$)="AB"} : !Check{fs{a$}="[ab]"}
  !Check{a$<a$+"c"} : !Check{e$<a$} : !Check{e$=""}
  !Check{a$<=a$} : !Check{a$>=a$}
  r=(a$=b$) : !CheckEq{r,0,"false"}
  r=(a$<b$) : !CheckEq{r,-1,"true"}
  r=1+(a$<b$) : !CheckEq{r,0,"in_arithmetic"}
  r=(a$<b$)*3+(b$<a$) : !CheckEq{r,-3,"two_in_arithmetic"}
!EndTestCase

!TestCase{logic}
  a$="ab" : b$="cd"
  r=a$="ab" AND b$="cd" : !CheckEq{r,-1,"and"}
  r=a$="ab" AND b$="ab" : !CheckEq{r,0,"and_false"}
  r=a$="x" OR b$>="cd" : !CheckEq{r,-1,"or"}
  r=(a$<b$) AND (a$+b$<>"") OR (a$="z") : !CheckEq{r,-1,"and_or"}
  r=NOT a$=b$ : !CheckEq{r,-1,"not"}
  If a$<b$ AND b$<>"" Then r=5 Else r=6
  !CheckEq{r,5,"if_and"}
  If a$="z" OR fs{b$}="[cd]" Then r=7 Else r=8
  !CheckEq{r,7,"if_or_function"}
!EndTestCase

!TestCase{parameters}
  a$="ab" : b$="cd"
  !CheckStrEq{fs{a$+"x"},"[abx]","expression"}
  !CheckStrEq{f2s{a$+b$,"c"+Str$(1)},"abcd/c1","two"}
  !CheckStrEq{fs{fs{a$}+b$},"[[ab]cd]","nested"}
  !CheckEq{fn{a$+b$+"e"},10,"numeric_function"}
  !CheckEq{fsn{a$+b$,Len(b$)},6,"string_and_number"}
  r=1+(2+fn{fs{a$}}) : !CheckEq{r,11,"right_operand"}
  c$="<"+f2s{fs{a$},UCase$(b$)}+">" : !CheckStrEq{c$,"<[ab]/CD>","in_expression"}
!EndTestCase

!TestCase{two_in_a_statement}
  a$="ab" : b$="cd"
  !CheckEq{Instr(a$+b$,"c"+"d"),3,"instr"}
  c$=Left$(a$+b$,Len(b$+"x")) : !CheckStrEq{c$,"abc","left_len"}
  c$=Mid$(a$+b$,Instr(b$+a$,"a"),2) : !CheckStrEq{c$,"cd","mid_instr"}
!EndTestCase

!TestCase{compiled}
  never=0 : a$="ab" : b$="cd"
  If never=1 Then NPrint a$+b$,"x"+a$,Len(a$)
  If never=1 Then NPrint fs{a$}+"x",a$<b$
  If never=1 Then l=a$
  !CheckEq{never,0,"not_run"}
!EndTestCase

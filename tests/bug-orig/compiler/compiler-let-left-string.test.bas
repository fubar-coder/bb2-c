; compiler-let-left-string - an assignment to a string array element (or
; to a string field of a NEWTYPE array element) runs the right side
; first: its finished string lies at the start of the string work area,
; a3 points to it and its length is on the stack (dolet2, blitz2.s:14340,
; .nextcom, moved0a3).  The index code runs after it and starts its own
; string work at the same place: a library function with a string
; argument (Val(d$), Mid$, Instr), a string comparison or a Function that
; takes, builds or returns strings overwrites the right side's string,
; and a Function with a local string leaves a3 elsewhere.  The element
; gets the right length with the wrong characters.
; Correct: every element holds "abcde".  The stored string is compared by
; the codes of its characters (codes{}), as a wrong one holds NUL bytes.
DEFTYPE .l
NEWTYPE.nt
  k.l
  n$
End NEWTYPE
NEWTYPE.nt2
  n$
  m$
End NEWTYPE

Function$ codes{t$}
  r$=""
  For k=1 To Len(t$)
    r$=r$+Str$(Asc(Mid$(t$,k,1)))+","
  Next
  Function Return r$
End Function
Function.l fnone{i}
  Function Return i
End Function
Function.l fstr{i}
  a$=Str$(i)+"xyzxyzxyz"
  Function Return i
End Function
Function.l ftmp{i}
  Function Return i+Len(Str$(i)+"q")-2
End Function
Function.l fpar{i,p$}
  Function Return i+Len(p$)-1
End Function
Function.l fset{i}
  Dim w$(4) : d$="3" : e$="fg"
  w$(Val(d$))="x"+e$
  Function Return i
End Function
Function$ fs{i}
  Function Return Str$(i)+"zz"
End Function

!TestCase{lib_index}
  Dim s$(4) : c$="cde" : d$="2"
  s$(Val(d$))="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{instr_index}
  Dim s$(4) : c$="cde"
  s$(Instr(c$,"d"))="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{compare_index}
  Dim s$(4) : c$="cde"
  s$(2+(c$="x"))="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{function_index}
  Dim s$(4) : c$="cde"
  s$(ftmp{2})="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{parameter_index}
  Dim s$(4) : c$="cde"
  s$(fpar{2,"q"})="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{string_function_index}
  Dim s$(4) : c$="cde"
  s$(Len(fs{2})-1)="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{field}
  Dim p.nt(4) : c$="cde"
  p(Val(Mid$("123",2,1)))\n="ab"+c$
  !CheckStrEq{codes{p(2)\n},"97,98,99,100,101,","stored"}
!EndTestCase

; a Function with a local string in the index: the originals copy from
; where the Function left a3
!TestCase{local_string}
  Dim s$(4) : c$="cde"
  s$(fstr{2})="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
  !CheckEq{Len(s$(2)),5,"length"}
!EndTestCase
!TestCase{local_string_literal}
  Dim s$(4)
  s$(fstr{2})="t"
  !CheckStrEq{s$(2),"t","stored"}
!EndTestCase
!TestCase{local_string_long}
  Dim s$(4) : c$="cde"
  s$(fstr{2})="abcdefghijklmnopqrstuvwxyz"+c$
  !CheckStrEq{codes{s$(2)},codes{"abcdefghijklmnopqrstuvwxyzcde"},"stored"}
!EndTestCase
!TestCase{local_string_field}
  Dim p.nt(4) : c$="cde"
  p(fstr{2})\n="ab"+c$
  !CheckStrEq{codes{p(2)\n},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{local_string_two_dims}
  Dim u$(2,4) : c$="cde"
  u$(1,fstr{2})="ab"+c$
  !CheckStrEq{codes{u$(1,2)},"97,98,99,100,101,","stored"}
!EndTestCase

; a Function in the index that assigns a string element with a string
; index itself; the comma form of a NEWTYPE element's fields
!TestCase{nested}
  Dim s$(4) : c$="cde"
  s$(fset{2})="ab"+c$
  !CheckStrEq{codes{s$(2)},"97,98,99,100,101,","stored"}
!EndTestCase
!TestCase{comma_form}
  Dim q.nt2(4) : c$="cde" : d$="2"
  q(Val(d$))\n="ab"+c$,c$+"fg"
  !CheckStrEq{codes{q(2)\n},"97,98,99,100,101,","first"}
  !CheckStrEq{q(2)\m,"cdefg","second"}
!EndTestCase

; not affected: an index without strings, a numeric array, the index
; computed first
!TestCase{plain_index}
  Dim s$(4) : c$="cde" : i=1
  s$(i+1)="ab"+c$
  s$(fnone{3})=Mid$(c$+"fgh",2,3)
  !CheckStrEq{s$(2),"abcde","stored"}
  !CheckStrEq{s$(3),"def","function"}
!EndTestCase
!TestCase{numeric_array}
  Dim a(4) : c$="cde" : d$="2"
  a(fstr{2})=Len("ab"+c$)
  a(Val(d$)+1)=Len(c$+"fghi")
  !CheckEq{a(2),5,"function"}
  !CheckEq{a(3),7,"library"}
!EndTestCase
!TestCase{index_first}
  Dim s$(4) : c$="cde"
  i=fstr{2} : s$(i)="ab"+c$
  !CheckStrEq{s$(2),"abcde","stored"}
  !CheckStrEq{s$(1),"","before"}
  !CheckStrEq{s$(3),"","after"}
!EndTestCase

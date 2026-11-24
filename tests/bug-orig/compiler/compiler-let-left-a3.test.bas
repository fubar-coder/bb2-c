; compiler-let-left-a3 - an assignment to a numeric array element (or to a
; numeric field of a NEWTYPE array element) whose index uses the string
; work area (a library function with a string argument, a string
; comparison, a Function with a string parameter): the original compiles
; the right side with the string state the index left (dolet2,
; blitz2.s:14397-14416, makesbase :10305), so the right side's string is
; built at whatever a3 holds, as the right side runs first.  The value
; stored is right, the memory at a3 is overwritten: after Read t$ a3
; points into the program's Data, behind the item read, and the next item
; is overwritten; GetReg a3 sets a3 to a buffer filled with ".".
; Correct: the memory at a3 is not changed.  The bytes are compared by
; their codes (mc{}), as a NUL cannot be written in a string predicate.
DEFTYPE .l
NEWTYPE.nt
  k.l
  n$
End NEWTYPE

Function$ mc{p}
  r$=""
  For k=0 To 11
    r$=r$+Str$(Peek.b(p+k))+","
  Next
  Function Return r$
End Function
Function.l fpar{i,p$}
  Function Return i+Len(p$)-1
End Function

!TestCase{after_read}
  Dim a(4) : c$="cde" : d$="2" : t$="" : r=0
  Goto skip
  d1: Data$ "JKL","MNOPQRST"
  .skip
  Restore d1
  Read t$
  PutReg a3,r
  m1$=mc{r-4}
  GetReg a3,r
  a(Val(d$))=Len(c$+"xyz")
  !CheckEq{a(2),6,"stored"}
  !CheckStrEq{mc{r-4},m1$,"data"}
!EndTestCase
!TestCase{after_read_instr}
  Dim a(4) : c$="cde" : d$="2" : t$="" : r=0
  Goto skip2
  d2: Data$ "JKL","MNOPQRST"
  .skip2
  Restore d2
  Read t$
  PutReg a3,r
  m1$=mc{r-4}
  GetReg a3,r
  a(Instr(c$,"d"))=Asc("Z"+c$)
  !CheckEq{a(2),90,"stored"}
  !CheckStrEq{mc{r-4},m1$,"data"}
!EndTestCase
!TestCase{filled}
  Dim a(4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  a(Val(d$))=Len(c$+"x")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{instr_right}
  Dim a(4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  a(Val(d$))=Instr(c$+"y","y")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{compare_index}
  Dim a(4), m.b(40) : c$="cde"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  a(2+(c$="x"))=Len(c$+"x")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{parameter_index}
  Dim a(4), m.b(40) : c$="cde"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  a(fpar{2,"q"})=Len(c$+"x")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{field}
  Dim p.nt(4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  p(Val(d$))\k=Len(c$+"x")
  !CheckEq{p(2)\k,4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{two_dims}
  Dim b(4,4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  b(1,Val(d$))=Len(c$+"x")
  !CheckEq{b(1,2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase

; not affected: an index without strings, the index computed first
!TestCase{plain_index}
  Dim a(4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  q=&m(0)
  GetReg a3,q
  a(2)=Len(c$+"x")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase
!TestCase{index_first}
  Dim a(4), m.b(40) : c$="cde" : d$="2"
  For k=0 To 39 : m(k)=46 : Next
  i=Val(d$)
  q=&m(0)
  GetReg a3,q
  a(i)=Len(c$+"x")
  !CheckEq{a(2),4,"stored"}
  !CheckStrEq{mc{q},"46,46,46,46,46,46,46,46,46,46,46,46,","memory"}
!EndTestCase

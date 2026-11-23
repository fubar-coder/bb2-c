; stringfunclib-cvi-length (#288) - Cvi always reads 2 bytes of its
; string, Cvl and Cvq 4, without looking at the string's length: a shorter
; string gets the bytes behind it (its terminating 0, then what an earlier
; expression left in the string buffer).  Before each call
; Len("WXYZWXYZ") leaves these characters at the start of the buffer.
; Correct: only the string's own bytes count, the missing ones are 0:
; Cvl("a") = $61000000, Cvl("ab") = $61620000, Cvl("") = 0, Cvi("") = 0,
; Cvi("a") = $6100.
DEFTYPE .l

!TestCase{short_strings}
  x=Len("WXYZWXYZ") : v=Cvl("a")
  !CheckEq{v,$61000000,"cvl_a"}
  x=Len("WXYZWXYZ") : v=Cvl("ab")
  !CheckEq{v,$61620000,"cvl_ab"}
  x=Len("WXYZWXYZ") : v=Cvl("")
  !CheckEq{v,0,"cvl_empty"}
  x=Len("WXYZWXYZ") : v=Cvi("")
  !CheckEq{v,0,"cvi_empty"}
  x=Len("WXYZWXYZ") : v=Cvi("a")
  !CheckEq{v,24832,"cvi_a"}
  x=Len("WXYZWXYZ") : v=Cvl("abcd")
  !CheckEq{v,$61626364,"cvl_abcd"}
  x=Len("WXYZWXYZ") : v=Cvi("ab")
  !CheckEq{v,24930,"cvi_ab"}
!EndTestCase

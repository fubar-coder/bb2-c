; model-numbering: the numbers of the checks of a test of the BASIC test
; model (tests/README.md, "Tests that judge themselves"). The runner's
; preprocessor numbers the checks over the whole source and keeps their
; table on the host; the library writes the number with each entry. Every
; check here expects a value of its own, and model-numbering.test.toml
; asserts the expected value of each check by its name: an entry that the
; table gave to another check fails that assertion. The source has checks
; in a procedure called by both cases, in a function, in a loop, in the
; batch init and end, a case behind CNIF and a check inside CNIF that is
; not compiled; it runs with blitz2 1.95, 2.10 and the port, with the CD's
; and the C libraries (basic-test-runner task 2.2: the numbering is stable
; for the three compilers).
#yes=1
Statement count_up{n.l}
  !CheckEq{n,n,"arg"}
End Statement
Function.l twice{n.l}
  !CheckEq{n+200,n+200,"inner"}
  Function Return n*2
End Function

!TestBatchInit
  base.l=10
  !CheckEq{base,10,"base"}
!EndTestBatchInit

!TestCase{first}
  !CheckEq{101,101,"one"}
  For i.l=1 To 3
    !CheckEq{110+i,110+i,"loop"}
  Next
  count_up{5}
  !CheckEq{twice{2},4,"twice"}
  !CheckStrEq{"a"+"b","ab","str"}
!EndTestCase

CNIF #yes=1
!TestCase{second}
  !CheckEq{102,102,"two"}
  CNIF #yes=0
  !CheckEq{109,109,"skipped"}       ; not compiled: no entry
  CEND
  count_up{7}
  !Check{base=10,"base_kept"}
!EndTestCase
CEND

!TestBatchEnd
  !CheckEq{base+990,1000,"final"}
!EndTestBatchEnd

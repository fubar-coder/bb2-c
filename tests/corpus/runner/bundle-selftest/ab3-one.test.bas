; ab3-one: a case of blitz2-ab3 in AB3 mode beside a cell of AmiBlitz3 (the
; side oracle, never bundled), a member of a bundle of the cases of ab3
; (change ab3-test-model D14); the check continued with @@
!TestCase{one}
  a.l=40
  !CheckEq{a+ @@
    2,42,"sum"}
!EndTestCase

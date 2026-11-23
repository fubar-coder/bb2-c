; riamosfunclib-banklimit-result (#483) - BankLimit's syntax in
; RIAmosFuncLib's manual is "[numbanks=]BankLimit[(maxbanks)]": the form
; with an argument returns the number of banks, as the form without one.
; The original returns what its init leaves in d0, the address of the new
; start table.
; Correct: BankLimit(60) returns 60, BankLimit then says 60 too.
DEFTYPE .l

!TestCase{banklimit_result}
  s=BankLimit(60)
  !CheckEq{s,60,"result"}
  b=BankLimit
  !CheckEq{b,60,"again"}
!EndTestCase

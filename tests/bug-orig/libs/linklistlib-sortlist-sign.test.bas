; linklistlib-sortlist-sign (#201) - SortList compares two keys with
; CMP.l and BMI, the sign of their difference: for keys more than 2^31
; apart the difference overflows and the order is wrong
; (2000000000 - -2000000000 is negative as a long).
; Correct: ascending signed order.
DEFTYPE .l

.keys
Data.l 2000000000,-2000000000,5,-2147483648,2147483647

!TestCase{key_order}
  Dim List a.l(5)
  Dim v.l(4)
  Restore keys
  For i=0 To 4 : Read k : r=AddLast(a()) : a()=k : Next
  SortList a(),0
  i=0 : ResetList a()
  While NextItem(a()) : v(i)=a() : i+1 : Wend
  !CheckEq{v(0),-2147483648,"item0"}
  !CheckEq{v(1),-2000000000,"item1"}
  !CheckEq{v(2),5,"item2"}
  !CheckEq{v(3),2000000000,"item3"}
  !CheckEq{v(4),2147483647,"item4"}
!EndTestCase

; linklistlib-sortlist-sign (#201) - SortList compares two keys with
; CMP.l and BMI, the sign of their difference: for keys more than 2^31
; apart the difference overflows and the order is wrong
; (2000000000 - -2000000000 is negative as a long).
; Correct: ascending signed order.
; @fixed #201
; @configs clib
DEFTYPE .l
Dim List a.l(5)
Restore keys
For i=1 To 5 : Read k : r=AddLast(a()) : a()=k : Next
SortList a(),0
Print "sorted" : ResetList a()
While NextItem(a()) : Print " ",a() : Wend
NPrint ""
End
keys:
Data.l 2000000000,-2000000000,5,-2147483648,2147483647

; riamosfunclib-reserve-negative-length (#500) - Reserve(length) returns
; the number of the bank it reserved, or -1 for failure (RIAmosFuncLib's
; manual).  For a negative length it returns 0 (BMI oops), the number of
; the first bank: the program takes bank 0 as reserved.
; Correct: Reserve(-5) returns -1 and reserves nothing.
; @fixed #500
; @rwe riamosfunclib
DEFTYPE .l
r=Reserve(-5)
NPrint r
If Start(0)=0 Then NPrint "bank 0 free" Else NPrint "bank 0 used"
End

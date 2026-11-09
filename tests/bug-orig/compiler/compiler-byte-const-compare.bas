; compiler-byte-const-compare (#978) - a byte variable is fetched with
; ext.w ($90 becomes $FF90), a constant from 128 to 255 is taken as a
; positive word ($0090), and cmp.w compares the two: the comparison with
; the constant never matches, on either side of the operator.
; Correct: a.b=$90 equals $90 (as it equals -112), $90 equals a, a is not
; different from 144; the assignment of 200 to a byte stays -56; the
; ordering comparisons stay mathematical: a<200 is true for a=$90 (-112)
; and for a=0.
; @fixed #978
a.b=$90
If a=$90 Then NPrint "ok" Else NPrint "fail"
If a=-112 Then NPrint "ok" Else NPrint "fail"
If $90=a Then NPrint "ok" Else NPrint "fail"
If a<>144 Then NPrint "fail" Else NPrint "ok"
If a<200 Then NPrint "ok" Else NPrint "fail"
a=0
If a<200 Then NPrint "ok" Else NPrint "fail"
c.b=200
NPrint c
End

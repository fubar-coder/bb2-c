; compiler-byte-const-compare (#978) - a byte variable is fetched with
; ext.w ($90 becomes $FF90), a constant from 128 to 255 is taken as a
; positive word ($0090), and cmp.w compares the two: the comparison with
; the constant never matches.
; Correct: a.b=$90 equals $90 (as it equals -112).
a.b=$90
If a=$90 Then NPrint "ok" Else NPrint "fail"
If a=-112 Then NPrint "ok" Else NPrint "fail"
End

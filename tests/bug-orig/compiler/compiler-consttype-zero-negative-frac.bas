; compiler-consttype-zero-negative-frac (#13) - consttype2 builds a Quick
; constant from the integer part and the fraction * 65536, and takes a
; fraction of 0 (beq) or a negative one (unsigned bcc) for one that does
; not fit: "q=1.0" compiles a float constant and its conversion to Quick
; where "q=1.5" is a Quick constant. (A literal has no sign, so a negative
; fraction does not come from a source.)
; Correct: 1.0 is the Quick constant 1, the code of q=1.0 is as long as
; that of q=1.5.
; @fixed #13
l1:q=1.0
NPrint q
l2:q=1.5
NPrint q
l3:
If ?l2-?l1=?l3-?l2 Then NPrint "same" Else NPrint "differ"
End

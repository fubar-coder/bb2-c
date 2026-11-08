; compiler-object-index-word (#939) - blitz2 addresses an object as the
; object table base + n << shift, with the shifted number as a word
; (lsl.w, add.w): from n << shift = $8000 on the offset is negative.  The
; shape maximum is raised to 1100 (1100 * 32 bytes, the table is
; allocated with the long size); Addr Shape(1025) is then 32736 bytes
; before the table instead of 32800 behind it.
; Correct: Addr Shape(1025) - Addr Shape(0) = 1025 * 32 = 32800.
; @fixed #939
; @pass jobline=max=154|1100
DEFTYPE .l
a.l=Addr Shape(1025)-Addr Shape(0)
NPrint a
End

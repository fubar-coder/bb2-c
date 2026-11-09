; compiler-consttype-d0-unloaded (#12) - a port regression, not a bug of
; the original: for a float literal in a Word expression or in one
; without a type yet, consttype2 jumps to .quick2 without makeint, so d0
; is not the integer part. In the original 1.95 the upper word of d0
; still holds the upper word of the last FFP result (negative), the range
; check fails and the literal is always a float constant. The port passed
; the character after the number with an upper word of 0: "a.w=0.5" at
; the end of the line (character 0) became the Quick fraction $8000
; (-32768), "42.5*" (character '*' = 42) and "43.5+" became Quick
; constants by chance (42.5*1000 overflowed the Quick).
; Expected (what the original does): the float in both cases: a.w=0.5 is
; 0, b.w=1.5 is 1, 42.5*1000 is 42500, and the untyped literals 2.5 and
; 43.5 compile alike (the same code size).
a.w=0.5
NPrint a
b.w=1.5
NPrint b
NPrint 42.5*1000
f.f=1
l1:NPrint 2.5+f
l2:NPrint 43.5+f
l3:
If ?l2-?l1=?l3-?l2 Then NPrint "same" Else NPrint "differ"
End

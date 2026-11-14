; printlib-ffptoascii-tiny-overflow (#241) - the fixed point form of
; ffptoascii (FloatMode -1, or any Format string) writes the digits of a
; number below 1E-17 behind its 50 byte buffer, into the first entry
; (1E19) of the table of powers of ten behind it: numbers of 1E18 and
; more print wrong digits afterwards.
; Here two tiny numbers with FloatMode -1, then 1E18, 5E18 and 9E18 in
; the normal form.
; Correct: the tiny numbers in full and the large ones as without them
; (9E+18 is the literal 9.223371E+18, the largest FFP number).
; @fixed #241
; @configs clib
f.f=0
FloatMode -1
Poke.l &f,$80000080 : NPrint "a <",f,">"
Poke.l &f,$C0000002 : NPrint "b <",f,">"
FloatMode 0
NPrint "c <",1E+18,"> <",5E+18,"> <",9E+18,">"
End

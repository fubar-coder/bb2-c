; compiler-newtype-forward-resident (#974) - a resident with the types of
; compiler-newtype-forward-transitive (.tb contains the forward .tc by
; value, .ta contains .tb): the resident is written after .tb and .ta are laid
; out again.
; Correct: the program that loads the resident sees .tb with 16 bytes (z at
; 14) and .ta with 20 (q at 16).
; @fixed #974
; @pass file=r.bb2|@TESTDATA@/compiler-newtype-forward-resident.res
; @pass jobline=resident=r|CORPUS:r.bb2|CORPUS:out/r.res
DEFTYPE .ta v
v\m\x\t=7
v\q=9
NPrint "b ",SizeOf .tb," z ",SizeOf .tb\z
NPrint "a ",SizeOf .ta," q ",SizeOf .ta\q
NPrint v\m\x\t," ",v\q
End

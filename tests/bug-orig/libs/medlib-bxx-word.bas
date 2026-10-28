; medlib-bxx-word (#203) - the position jump Bxx stores the whole word of
; d4, which is cleared once per line and then loaded bytewise per track:
; its upper byte is left over from a track before it in the same line.
; The module here (generated) has 19 08 (sample offset: d4 = $0800) in
; track 0 and 0B 05 in track 1 of its only line: the jump stores $0805,
; at the change of the block that is not below the song length (8), the
; player takes position 0 and writes only the low byte - the position
; word of the module (+46, mmd_pseqnum) stays $0800.  PlayMed plays one
; line (the first PlayMed after StartMedModule starts a line).
; Correct: the jump goes to position 5.
; @configs clib
; @pass file=bxx.med|@TESTDATA@/medlib-bxx-word.med
DEFTYPE .l
LoadMedModule 0,"CORPUS:bxx.med"
d=Peek.l(Addr MedModule(0)+8)
StartMedModule 0
PlayMed
NPrint "position ",Hex$(Peek.w(d+46))," block ",Peek.w(d+42)," line ",Peek.w(d+44)
StopMed
End

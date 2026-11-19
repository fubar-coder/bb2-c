; ribbmaplib-shadowvalue-past-shdw (#1114) - BBShadowValue returns the
; long at SHDW chunk + set * 4 without comparing the set with the
; chunk's length.  The scenario (built in memory) has a SHDW chunk of two
; sets ($11111111, $22222222); the chunk "NEXT" follows.  Set 2 is an
; invalid argument (D-005); the original has no error check area and
; reads the next chunk's id ($4E455854).  Here with runtime error
; checking.
; Correct: the error check refuses set 2: "Shadow set must lie in the
; SHDW chunk".
; @fixed #1114
; @rwe ribbmaplib
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,32 : Poke.l m+8,$5343454E
Poke.l m+12,$53484457 : Poke.l m+16,8 : Poke.l m+20,$11111111 : Poke.l m+24,$22222222
Poke.l m+28,$4E455854 : Poke.l m+32,4 : Poke.l m+36,$33333333
NPrint SetBBScenarioLoc(m)
NPrint Hex$(BBShadowValue(1))
NPrint Hex$(BBShadowValue(2))
FreeMem_ m,64
End

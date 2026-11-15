; medlib-getmedvolume-a3 (#1061) - GetMedVolume, GetMedNote and GetMedInstr
; load a3 from the player's structure (MOVEA.l (a2),a3) and do not
; restore it.  a3 is the write pointer of the string work area: in a string
; expression the next part is built wherever a3 points now.
; Correct: "V"+Str$(GetMedVolume(0)) is "V0" (no module played: volume 0),
; and the same for GetMedNote and GetMedInstr.
; @fixed #1061
; @configs clib
a$="V"+Str$(GetMedVolume(0))
NPrint Len(a$)," ",a$
b$="N"+Str$(GetMedNote(0))
NPrint Len(b$)," ",b$
c$="I"+Str$(GetMedInstr(0))
NPrint Len(c$)," ",c$
End

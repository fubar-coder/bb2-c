; medlib-getmedvolume-a3 (#1061) - GetMedVolume, GetMedNote and GetMedInstr
; load a3 from the player's structure (MOVEA.l (a2),a3) and do not
; restore it.  a3 is the write pointer of the string work area: in a string
; expression the next part is built wherever a3 points now.
; Here: "V"+Str$(GetMedVolume(0)), and the same for GetMedNote and
; GetMedInstr, each in a case of its own.
; Correct: the string is "V0" (no module played: volume 0), and so
; "N0" and "I0".
DEFTYPE .l

!TestCase{get_volume}
  a$="V"+Str$(GetMedVolume(0))
  l=Len(a$)
  !CheckEq{l,2,"length"}
  !CheckStrEq{a$,"V0","text"}
!EndTestCase

!TestCase{get_note}
  b$="N"+Str$(GetMedNote(0))
  l=Len(b$)
  !CheckEq{l,2,"length"}
  !CheckStrEq{b$,"N0","text"}
!EndTestCase

!TestCase{get_instr}
  c$="I"+Str$(GetMedInstr(0))
  l=Len(c$)
  !CheckEq{l,2,"length"}
  !CheckStrEq{c$,"I0","text"}
!EndTestCase

; menuslib-subitem-replace-shift (#220) - SubItem replacing an existing
; subitem moves the following ones by (subitem number - old height)
; instead of the height difference: after textcalc, MOVEM restores d3,
; the subitem number, where menucont expects the new height.  Three
; subitems of the same height; subitem 0 and then subitem 1 are replaced
; by subitems of the same height; the TopEdge (+6) of the three before
; and after.
; Correct: the heights do not change, so no subitem moves.
DEFTYPE .l

; the TopEdge of the three subitems -> t1, t2, t3
Statement tops{}
  SHARED t1,t2,t3
  s=Peek.l(Peek.l(Peek.l(Addr MenuList(0))+18)+28)
  t1=Peek.w(s+6) : t2=Peek.w(Peek.l(s)+6) : t3=Peek.w(Peek.l(Peek.l(s))+6)
End Statement

!TestCase{replace_subitems}
  MenuTitle 0,0,"Project"
  MenuItem 0,0,0,0,"Item"
  SubItem 0,0,0,0,0,"A"
  SubItem 0,0,0,0,1,"B"
  SubItem 0,0,0,0,2,"C"
  tops{}
  !CheckEq{t1,4,"initial_1"}
  !CheckEq{t2,16,"initial_2"}
  !CheckEq{t3,28,"initial_3"}
  SubItem 0,0,0,0,0,"AA"
  tops{}
  !CheckEq{t1,4,"after_aa_1"}
  !CheckEq{t2,16,"after_aa_2"}
  !CheckEq{t3,28,"after_aa_3"}
  SubItem 0,0,0,0,1,"BB"
  tops{}
  !CheckEq{t1,4,"after_bb_1"}
  !CheckEq{t2,16,"after_bb_2"}
  !CheckEq{t3,28,"after_bb_3"}
!EndTestCase

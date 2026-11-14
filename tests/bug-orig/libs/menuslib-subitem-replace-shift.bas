; menuslib-subitem-replace-shift (#220) - SubItem replacing an existing
; subitem moves the following ones by (subitem number - old height)
; instead of the height difference: after textcalc, MOVEM restores d3,
; the subitem number, where menucont expects the new height.  Three
; subitems of the same height; subitem 0 and then subitem 1 are replaced
; by subitems of the same height; the TopEdge (+6) of the three before
; and after.
; Correct: the heights do not change, so no subitem moves.
; @fixed #220
; @configs clib
DEFTYPE .l
MenuTitle 0,0,"Project"
MenuItem 0,0,0,0,"Item"
SubItem 0,0,0,0,0,"A"
SubItem 0,0,0,0,1,"B"
SubItem 0,0,0,0,2,"C"
Gosub tops
SubItem 0,0,0,0,0,"AA"
Gosub tops
SubItem 0,0,0,0,1,"BB"
Gosub tops
End
tops:
  s=Peek.l(Peek.l(Peek.l(Addr MenuList(0))+18)+28)
  NPrint "top ",Peek.w(s+6)," ",Peek.w(Peek.l(s)+6)," ",Peek.w(Peek.l(Peek.l(s))+6)
Return

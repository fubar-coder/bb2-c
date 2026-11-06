; menuslib-empty-item-text (#981) - textcalc copies the text of an item
; with allocst, which returns 0 for a text of length 0; the 0 goes into
; the IntuiText's IText and IntuiTextLength reads the string at address 0:
; the item's width is that of whatever is there.
; Here the long at address 0 is set to "ABC" (restored afterwards), so
; the width shows the read: 2 * MenuGap + 3 characters instead of
; 2 * MenuGap. Item and subitem. (MenuTitle with an empty title also stores
; MenuName 0, but its width is computed from the length alone: not judged.)
; Correct: an empty text gives IText a string (not 0) and the width of
; the gaps alone.
DEFTYPE .l
MenuTitle 0,0,"Project"
z=Peek.l(0)
Poke.l 0,$41424300
MenuItem 0,0,0,0,""
SubItem 0,0,0,0,0,""
Poke.l 0,z
m=Peek.l(Addr MenuList(0))
i=Peek.l(m+18)
s=Peek.l(i+28)
NPrint "item width ",Peek.w(i+8)," IText nonzero ",-(Peek.l(i+46)<>0)
NPrint "subitem width ",Peek.w(s+8)," IText nonzero ",-(Peek.l(s+46)<>0)
End

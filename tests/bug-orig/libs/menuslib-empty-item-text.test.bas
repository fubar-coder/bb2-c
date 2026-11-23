; menuslib-empty-item-text (#981) - textcalc copies the text of an item
; with allocst, which returns 0 for a text of length 0; the 0 goes into
; the IntuiText's IText and IntuiTextLength reads the string at address 0:
; the item's width is that of whatever is there.
; Here the long at address 0 is set to "ABC" (restored afterwards), so
; the width shows the read: 2 * MenuGap + 3 characters instead of
; 2 * MenuGap. Item and subitem. MenuTitle with an empty title also stores
; MenuName 0 (Intuition reads the title there; AmiBlitz 2.42 lists "fixed
; harmless enforcer hit on menutitles with string """); its width is
; computed from the length alone.
; Correct: an empty text gives IText and MenuName a string (not 0) and the
; item the width of the gaps alone.
DEFTYPE .l

!TestCase{empty_texts}
  MenuTitle 0,0,"Project"
  z=Peek.l(0)
  Poke.l 0,$41424300
  MenuItem 0,0,0,0,""
  SubItem 0,0,0,0,0,""
  Poke.l 0,z
  MenuTitle 0,1,""
  m=Peek.l(Addr MenuList(0))
  i=Peek.l(m+18)
  s=Peek.l(i+28)
  !CheckEq{Peek.w(i+8),8,"item_width"}
  !Check{Peek.l(i+46)<>0,"item_itext_set"}
  !CheckEq{Peek.w(s+8),8,"subitem_width"}
  !Check{Peek.l(s+46)<>0,"subitem_itext_set"}
  t=Peek.l(m)
  !CheckEq{Peek.w(t+8),8,"title_width"}
  !Check{Peek.l(t+14)<>0,"title_name_set"}
!EndTestCase

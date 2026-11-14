; menuslib-menuchecked-sub-loops (#217) - MenuChecked(MenuList#,Menu,Item,
; Subitem) walks to the subitem with three DBF loops, but the item and the
; subitem loop both jump back to the menu loop: only (menu,0,0) ends at the
; right entry; any other item or subitem walks the lists again with a
; counter of -1 (up to 65536 next pointers, from an item into whatever
; follows).  Item 1 of menu 0 has a checked subitem 0 (flags 3) and an
; unchecked subitem 1.
; Correct: (0,1,0) is checked (-1), (0,1,1) is not (0), (0,0,0) is not.
; @fixed #217
; @configs clib
DEFTYPE .w
MenuTitle 0,0,"Project"
MenuItem 0,0,0,0,"Open"
MenuItem 0,0,0,1,"Sub"
SubItem 0,0,0,0,0,"N"
SubItem 0,3,0,1,0,"A"
SubItem 0,0,0,1,1,"B"
NPrint "0,0,0 ",MenuChecked(0,0,0,0)
NPrint "1,0 ",MenuChecked(0,0,1,0)
NPrint "1,1 ",MenuChecked(0,0,1,1)
End

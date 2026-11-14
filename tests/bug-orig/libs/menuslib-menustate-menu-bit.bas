; menuslib-menustate-menu-bit (#216) - MenuState MenuList#,Menu,On|Off
; (a whole menu) goes through changestate2, which is meant for items, and
; sets or clears bit 4 of the Menu's Flags instead of MENUENABLED (bit 0):
; a single menu cannot be disabled or enabled.  The Flags word of menu 1
; (+12) after MenuState Off and On.
; Correct: Off clears MENUENABLED (flags 0), On sets it again (1); the
; other menu stays enabled.
; @fixed #216
; @configs clib
DEFTYPE .l
MenuTitle 0,0,"Project"
MenuTitle 0,1,"Edit"
m=Peek.l(Addr MenuList(0))
n=Peek.l(m)
MenuState 0,1,Off
NPrint "off ",Hex$(Peek.w(n+12))," other ",Hex$(Peek.w(m+12))
MenuState 0,1,On
NPrint "on ",Hex$(Peek.w(n+12))
End

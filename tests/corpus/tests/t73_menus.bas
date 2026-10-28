; t73_menus - menuslib: MenuTitle, MenuItem (flags 0-4, >=5, shortcut),
; SubItem, MenuGap, SubItemOff, MenuColour, replacing titles/items,
; MenuState, MenuChecked, SetMenu (width of the widest item); the menu
; structures are printed field by field
DEFTYPE .l
WbToScreen 0
Window 0,20,30,200,80,$000E,"Menus",1,2
DefaultOutput
MenuTitle 0,0,"Project"
MenuItem 0,0,0,0,"Open"
MenuItem 0,1,0,1,"Toggle","T"
MenuItem 0,2,0,2,"Excl A"
MenuItem 0,2,0,3,"Excl B"
MenuItem 0,3,0,4,"Checked"
MenuItem 0,4,0,5,"Excl C on"
MenuItem 0,$56,0,6,"Raw flags"
MenuItem 0,$10000,0,7,"Big flags"
MenuTitle 0,1,"Edit"
MenuGap 6,3 : MenuColour 3
MenuItem 0,0,1,0,"Cut","X"
SubItemOff 30,2
SubItem 0,0,1,0,0,"Sub one"
SubItem 0,1,1,0,1,"Sub two long","S"
MenuTitle 0,2,"Third"
MenuGap 4,2 : MenuColour 1 : SubItemOff 48,4
MenuItem 0,0,2,0,"Only"
Gosub dump
; replace: a longer title and a higher item move the following ones
MenuTitle 0,1,"Editing"
MenuItem 0,0,0,2,"Excl A again"
SubItem 0,0,1,0,0,"First sub"
NPrint "-- replaced"
Gosub dump
; MenuState / MenuChecked
MenuState 0,0,0,Off : MenuState 0,1,0,1,Off : MenuState 0,2,Off
MenuState 0,Off
NPrint "-- state"
Gosub dump
MenuState 0,On
a=MenuChecked(0,0,4) : b=MenuChecked(0,0,1) : c=MenuChecked(0,1,0,0) : d=MenuChecked(0,0,0)
NPrint "checked ",a," ",b," ",c," ",d
Use Window 0 : SetMenu 0
DefaultOutput
w=Peek.l(Addr Window(0))
NPrint "strip ",Peek.l(w+28)=Peek.l(Addr MenuList(0))
Gosub dump
SetMenu 0
DefaultOutput
NPrint "again ",Peek.l(w+28)=Peek.l(Addr MenuList(0))
CloseWindow 0
NPrint "list ",Peek.l(Addr MenuList(0))<>0," ",Peek.l(Addr MenuList(1))
End

dump:
  m=Peek.l(Addr MenuList(0)) : mn=0
  While m AND mn<10
    Print "menu ",mn," ",Peek.w(m+4)," ",Peek.w(m+6)," ",Peek.w(m+8)," "
    NPrint Peek.w(m+10)," ",Hex$(Peek.w(m+12))," [",Peek$(Peek.l(m+14)),"]"
    it=Peek.l(m+18) : itn=0
    While it AND itn<20
      Gosub item
      su=Peek.l(it+28) : sn=0
      While su AND sn<20
        Print "  sub ",sn," " : sv=it : it=su : Gosub item : it=sv
        su=Peek.l(su) : sn=sn+1
      Wend
      it=Peek.l(it) : itn=itn+1
    Wend
    m=Peek.l(m) : mn=mn+1
  Wend
Return

item:
  t=it+34
  Print "  item ",itn," ",Peek.w(it+4)," ",Peek.w(it+6)," ",Peek.w(it+8)," "
  Print Peek.w(it+10)," ",Hex$(Peek.w(it+12))," ",Hex$(Peek.l(it+14))," "
  NPrint Peek.l(it+18)=t," ",Peek.b(it+26)," ",Peek.w(it+32)
  Print "    text ",Peek.b(t)," ",Peek.b(t+1)," ",Peek.b(t+2)," ",Peek.w(t+4)," "
  NPrint Peek.w(t+6)," ",Peek.l(t+8)<>0," [",Peek$(Peek.l(t+12)),"] ",Peek.l(t+16)
Return

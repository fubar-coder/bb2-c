; t31_list - linklistlib: all tokens, empty/full lists, ends of lists,
; stale items after KillItem/ClearList, item stack, SortList
Dim List l.l(4)
; empty list
NPrint "empty ",FirstItem(l())," ",LastItem(l())," ",NextItem(l())," ",PrevItem(l())
ClearList l() : SortList l(),0 : ResetList l()
Gosub showl
; fill until full
For i=1 To 7
  r=AddItem(l()) : NPrint "add ",i," ",r
  If r Then l()=i*10
Next
Gosub showl : Gosub backl
NPrint "full ",AddFirst(l())," ",AddLast(l())," ",AddItem(l())
; ends
NPrint "first ",FirstItem(l())," ",l()," prev ",PrevItem(l())," ",l()
NPrint "last ",LastItem(l())," ",l()," next ",NextItem(l())," ",l()
NPrint "next ",NextItem(l())," ",l()
; kill the first item: no current item afterwards
r=FirstItem(l()) : KillItem l()
NPrint "kill1 ",NextItem(l())," ",l()
Gosub showl
; kill in the middle: current = previous one
r=FirstItem(l()) : r=NextItem(l()) : r=NextItem(l()) : NPrint "cur ",l()
KillItem l() : NPrint "after kill ",l()
Gosub showl
; kill the last one
r=LastItem(l()) : KillItem l() : NPrint "kill last ",l()
Gosub showl
; stale data: killed items come back with their old values
r=AddItem(l()) : NPrint "stale ",l() : r=AddItem(l()) : NPrint "stale ",l()
Gosub showl
; AddItem after current, AddFirst/AddLast make the new item current
ClearList l()
NPrint "cleared ",FirstItem(l())," ",LastItem(l())
r=AddItem(l()) : NPrint "c1 ",l() : l()=1
r=AddLast(l()) : NPrint "c2 ",l() : l()=2
r=AddFirst(l()) : NPrint "c3 ",l() : l()=3
r=FirstItem(l()) : AddItem l() : NPrint "c4 ",l() : l()=4
AddLast l() : l()=5
r=AddLast(l()) : NPrint "c6 ",r
Gosub showl : Gosub backl
; PrevItem without current = LastItem, NextItem without = FirstItem
ResetList l() : NPrint "rp ",PrevItem(l())," ",l()
ResetList l() : NPrint "rn ",NextItem(l())," ",l()
; item stack
r=FirstItem(l()) : r=NextItem(l()) : PushItem l()
r=LastItem(l()) : NPrint "push ",l()
PopItem l() : NPrint "pop ",l()
ResetList l() : PushItem l() : r=FirstItem(l()) : PopItem l()
NPrint "pop0 ",NextItem(l())," ",l()
Dim List m.w(3)
r=AddItem(m()) : m()=-7 : r=AddItem(m()) : m()=-8
r=FirstItem(l()) : PushItem l() : r=FirstItem(m()) : PushItem m()
r=LastItem(l()) : r=LastItem(m())
PopItem m() : PopItem l()
NPrint "pop2 ",l()," ",m()
ItemStackSize 3
For i=1 To 3 : r=FirstItem(l()) : For j=1 To i : r=NextItem(l()) : Next : PushItem l() : Next
For i=1 To 3 : PopItem l() : Print l()," " : Next : NPrint ""
ItemStackSize 1 : PushItem m() : PopItem m() : NPrint "st1 ",m()
; SortList on a NEWTYPE key
NEWTYPE .rec
  a.w
  k.l
  n.w
End NEWTYPE
Dim List s.rec(11)
Restore sdat
For i=1 To 10 : Read kk.l : r=AddLast(s()) : s()\a=i : s()\k=kk : s()\n=i*3 : Next
r=FirstItem(s()) : r=NextItem(s()) : r=NextItem(s())
NPrint "cur ",s()\a," ",s()\k
SortList s(),SizeOf .rec\k
NPrint "cur ",s()\a," ",s()\k
Gosub sortl
SortList s(),SizeOf .rec\a
Gosub sortl
SortList s(),0
Gosub sortl
; one and two items
Dim List t.l(3)
r=AddItem(t()) : t()=5 : SortList t(),0 : Gosub listt
r=AddItem(t()) : t()=-5 : SortList t(),0 : Gosub listt
r=AddFirst(t()) : t()=-9 : SortList t(),0 : Gosub listt
; ClearList sends items to the free list in reverse order
ClearList t()
For i=1 To 4 : r=AddLast(t()) : NPrint "t ",r," ",t() : Next
; kill the only item
Dim List u.l(0)
NPrint "u ",AddItem(u())," ",AddItem(u()) : u()=77
KillItem u() : NPrint "u ",FirstItem(u())," ",LastItem(u())," ",AddLast(u())," ",u()
End

showl:
Print "l:" : ResetList l()
While NextItem(l()) : Print " ",l() : Wend
NPrint ""
Return
backl:
Print "b:" : ResetList l()
While PrevItem(l()) : Print " ",l() : Wend
NPrint ""
Return
sortl:
ResetList s()
While NextItem(s()) : Print s()\a,":",s()\k," " : Wend
NPrint ""
Return
listt:
Print "t:" : ResetList t()
While NextItem(t()) : Print " ",t() : Wend
NPrint ""
Return

sdat:
Data.l 5,-3,2000000000,-2000000000,5,0,7,-1,100,2147483647

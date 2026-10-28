; t34_listerr3 - linklistlib error check: PopItem into another list
Dim List l.l(3)
Dim List m.l(3)
r=AddItem(l()) : PushItem l()
NPrint "before"
PopItem m()
NPrint "after"
End

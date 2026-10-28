; t35_listerr4 - linklistlib error check: item stack overflow
Dim List l.l(3)
ItemStackSize 2
r=AddItem(l()) : PushItem l() : PushItem l()
NPrint "before"
PushItem l()
NPrint "after"
End

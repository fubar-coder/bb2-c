; t33_listerr2 - linklistlib error check: PopItem on an empty item stack
Dim List l.l(3)
r=AddItem(l()) : PushItem l() : PopItem l()
NPrint "before"
PopItem l()
NPrint "after"
End

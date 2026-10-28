; t32_listerr1 - linklistlib error check: KillItem without current item
Dim List l.l(3)
r=AddItem(l()) : l()=1
ResetList l()
NPrint "before"
KillItem l()
NPrint "after ",l()
End

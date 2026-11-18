; rilinkedlistlib-addfirst-next (#724) - AddFirstListItem should make the
; new node the prev of the old first node; it writes it to offset 0 of
; that node, its next (the v0.9 layout had prev there).  The old first
; node's link to the second node is lost, and the new node and the old
; first node point to each other through their next fields.  The program
; builds A B, adds X in front and walks forward from the first node.
; Correct: X A B: the second NextListItem reaches B.
; @fixed #724
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
r.l=InitList(0,v)
r=AddLastListItem(0)
b.l=AddrListItem(0)
r=AddFirstListItem(0)
r=FirstListItem(0)
r=NextListItem(0)
r=NextListItem(0)
If AddrListItem(0)=b Then NPrint "B reached" Else NPrint "loop"
End

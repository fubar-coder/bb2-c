; rilinkedlistlib-addlistitem-prev (#727) - AddListItem inserts the new
; node behind the current one but does not set the prev of the node that
; was behind the current one: it still points to the old current node.
; The program builds A B, inserts X behind A, goes to B and back.
; Correct: PrevListItem from B reaches X.
; @fixed #727
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
r.l=InitList(0,v)
r=AddListItem(0)
r=FirstListItem(0)
r=AddListItem(0)
x.l=AddrListItem(0)
r=NextListItem(0)
r=PrevListItem(0)
If AddrListItem(0)=x Then NPrint "X reached" Else NPrint "X skipped"
End

; rilinkedlistlib-listnum-unchecked (#732) - the list number indexes the
; tables of five longs (first, last, an unused one, current, one after
; the other) without a range check: InitList 5 writes its node as the
; "first" of list 5, which is the last node of list 0.  The program
; prints whether LastListItem 0 still finds list 0's own node.
; Correct: InitList 5 is refused (0) and list 0 is left alone.
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
r.l=InitList(0,v)
a0.l=AddrListItem(0)
r=InitList(5,v)
NPrint r
r=LastListItem(0)
If AddrListItem(0)=a0 Then NPrint "list 0 kept" Else NPrint "list 0 changed"
End

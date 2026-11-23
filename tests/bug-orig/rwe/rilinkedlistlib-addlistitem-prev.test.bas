; rilinkedlistlib-addlistitem-prev (#727) - AddListItem inserts the new
; node behind the current one but does not set the prev of the node that
; was behind the current one: it still points to the old current node.
; The program builds A B, inserts X behind A, goes to B and back.
; Correct: PrevListItem from B reaches X.
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
DEFTYPE .l

!TestCase{insert}
  v\a=1
  r=InitList(0,v)
  r=AddListItem(0)
  r=FirstListItem(0)
  r=AddListItem(0)
  x=AddrListItem(0)
  r=NextListItem(0)
  r=PrevListItem(0)
  y=AddrListItem(0)
  !CheckEq{y,x,"prev_reaches_x"}
!EndTestCase

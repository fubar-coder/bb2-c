; rilinkedlistlib-size-field (#735) - the size field of a node (offset
; 14) does not match its data: AddListItem stores the size of the last
; variable any command looked at in a node without data, and SetListItem
; leaves the field as it was.  The program looks at a 12 byte variable
; (SizeOfListItem), adds a node and sets it from a 4 byte variable.
; Correct: 0 for the node without data, then 4.
NEWTYPE .small
  a.l
End NEWTYPE
NEWTYPE .big
  a.l
  b.l
  c.l
End NEWTYPE
DEFTYPE .small s
DEFTYPE .big b
DEFTYPE .l

!TestCase{node_size}
  r=InitList(0,s)
  n=SizeOfListItem(b)
  r=AddListItem(0)
  z1=Peek.l(AddrListItem(0)+14)
  r=SetListItem(0,s)
  z2=Peek.l(AddrListItem(0)+14)
  !CheckEq{z1,0,"without_data"}
  !CheckEq{z2,4,"after_set"}
!EndTestCase

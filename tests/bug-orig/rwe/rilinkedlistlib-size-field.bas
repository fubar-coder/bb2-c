; rilinkedlistlib-size-field (#735) - the size field of a node (offset
; 14) does not match its data: AddListItem stores the size of the last
; variable any command looked at in a node without data, and SetListItem
; leaves the field as it was.  The program looks at a 12 byte variable
; (SizeOfListItem), adds a node and sets it from a 4 byte variable.
; Correct: 0 for the node without data, then 4.
; @fixed #735
; @rwe rilinkedlistlib riamosfunclib
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
r.l=InitList(0,s)
n.l=SizeOfListItem(b)
r=AddListItem(0)
NPrint Peek.l(AddrListItem(0)+14)
r=SetListItem(0,s)
NPrint Peek.l(AddrListItem(0)+14)
End

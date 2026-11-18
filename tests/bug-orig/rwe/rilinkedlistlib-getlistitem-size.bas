; rilinkedlistlib-getlistitem-size (#730) - GetListItem copies as many
; bytes as the variable has, not as the node's copy has: into a larger
; variable it copies what lies behind the copy's allocation.  The program
; keeps a copy of a 4 byte variable and reads it into a 12 byte one.
; Correct: the first 4 bytes are copied, the rest of the variable keeps
; its contents.
; @fixed #730
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
s\a=1
b\b=$12345678
b\c=$9ABCDEF0
r.l=InitList(0,s)
r=GetListItem(0,b)
NPrint b\a
k.l=0
If b\b=$12345678
  If b\c=$9ABCDEF0 Then k=1
EndIf
If k Then NPrint "rest kept" Else NPrint "rest overwritten"
End

; rilinkedlistlib-getlistitem-size (#730) - GetListItem copies as many
; bytes as the variable has, not as the node's copy has: into a larger
; variable it copies what lies behind the copy's allocation.  The program
; keeps a copy of a 4 byte variable and reads it into a 12 byte one.
; Correct: the first 4 bytes are copied, the rest of the variable keeps
; its contents.
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

!TestCase{larger_variable}
  s\a=1
  b\b=$12345678
  b\c=$9ABCDEF0
  r=InitList(0,s)
  r=GetListItem(0,b)
  fa=b\a
  k=0
  If b\b=$12345678
    If b\c=$9ABCDEF0 Then k=1
  EndIf
  !CheckEq{fa,1,"first_field"}
  !CheckEq{k,1,"rest_kept"}
!EndTestCase

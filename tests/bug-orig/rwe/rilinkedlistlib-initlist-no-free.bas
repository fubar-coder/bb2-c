; rilinkedlistlib-initlist-no-free (#731) - InitList on a list that is
; already there overwrites its table entries without freeing the nodes
; and copies: each call loses them until the end.  The program
; initialises list 0 21 times with a 1004 byte variable and compares the
; free memory after the first and after the last call.
; Correct: the old list is freed, the free memory stays about the same.
; @fixed #731
; @rwe rilinkedlistlib riamosfunclib
NEWTYPE .t
  a.l
  b.b[1000]
End NEWTYPE
DEFTYPE .t v
v\a=1
DEFTYPE .l
r=InitList(0,v)
a=AvailMem_(0)
For i=1 To 20
  r=InitList(0,v)
Next
b=AvailMem_(0)
If a-b>10000 Then NPrint "old lists lost" Else NPrint "old lists freed"
End

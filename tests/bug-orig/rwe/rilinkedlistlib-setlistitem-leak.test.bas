; rilinkedlistlib-setlistitem-leak (#734) - SetListItem gives the current
; node a new copy of the variable without freeing the old one: each call
; on a node with data loses a copy until the end.  The program sets the
; node of list 0 20 times from a 1004 byte variable and compares the free
; memory before and after.
; Correct: the old copy is freed, the free memory stays about the same.
NEWTYPE .t
  a.l
  b.b[1000]
End NEWTYPE
DEFTYPE .t v
DEFTYPE .l

!TestCase{set_20_times}
  r=InitList(0,v)
  m0=AvailMem_(0)
  For i=1 To 20
    v\a=i
    r=SetListItem(0,v)
  Next
  m1=AvailMem_(0)
  d=m0-m1
  !Check{d<=10000,"old_copies_freed"}
!EndTestCase

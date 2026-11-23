; rilinkedlistlib-dellistitem-v09-layout (#729) - DelListItem reads the
; node in the layout of v0.9 (no pad word): its "data" is the long at
; offset 8, the pad and the upper word of the real data pointer.  It frees
; that address, which memlib does not know ("Unable to Free Memory"; the
; trap of that error is a crash without runtime error checking), and the
; copy is lost.  Here with runtime error checking; the program deletes
; the first node of A B, whose copy has 10004 bytes, and compares the
; free memory before and after.
; Correct: no error, DelListItem returns -1 and frees the copy.
NEWTYPE .t
  a.l
  b.b[10000]
End NEWTYPE
DEFTYPE .t v
DEFTYPE .l

!TestCase{delete_with_copy}
  v\a=1
  r=InitList(0,v)
  r=AddLastListItem(0)
  r=FirstListItem(0)
  m0=AvailMem_(0)
  r=DelListItem(0)
  m1=AvailMem_(0)
  d=m1-m0
  !CheckEq{r,-1,"result"}
  !Check{d>10000,"copy_freed"}
!EndTestCase

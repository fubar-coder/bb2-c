; rilinkedlistlib-addfirst-result (#725) - AddFirstListItem ends without
; setting its result: d0 is still the table offset list# * 4.  So a
; successful call on list 0 returns 0, the failure result, and on list 1
; 4.  The program adds a node in front of list 0 and of list 1.
; Correct: -1 for both, as the manual says.
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
DEFTYPE .l

!TestCase{results}
  v\a=1
  r=InitList(0,v)
  r=InitList(1,v)
  r0=AddFirstListItem(0)
  r1=AddFirstListItem(1)
  !CheckEq{r0,-1,"list_0"}
  !CheckEq{r1,-1,"list_1"}
!EndTestCase

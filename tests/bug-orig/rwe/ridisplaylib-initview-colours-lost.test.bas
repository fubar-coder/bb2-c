; ridisplaylib-initview-colours-lost (#890) - InitView's first
; instruction, MOVE.w d0,d3, overwrites the colour count with the height
; before it is read: the size of the copper list is 16 x numsprites +
; 16 x depth + 8 x height + 38, numcolours changes nothing.  The test
; makes two lists that differ only in numcolours (32 and 256) and takes
; how much more chip memory the second one took (with Forbid, so that no
; other task allocates in between).
; Correct: the 224 colours more take 8 bytes each: 1792 bytes more.
DEFTYPE .l

!TestCase{two_lists}
  Forbid_
  a=AvailMem_(2)
  c1=InitView(200,8,8,32)
  b=AvailMem_(2)
  c2=InitView(200,8,8,256)
  d=AvailMem_(2)
  Permit_
  !Require{c1<>0 AND c2<>0,"lists_made"}
  !CheckEq{(b-d)-(a-b),1792,"more_bytes"}
!EndTestCase

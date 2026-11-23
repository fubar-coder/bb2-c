; rigtmenulib-menuitem-form4-mutex (#692) - GTMenuItem with UserData
; (all eight arguments) jumps past the store of its mutualexclude: the
; item gets the mutual exclude of the GTMenuItem/GTSubItem before.  The
; program adds an item with mutual exclude 6, then one with 3 and
; UserData, and reads both masks from the object's list (the titles at
; +4 of the object, a title's first item at +8, an item's mask at +22,
; its UserData at +26).
; Correct: the second item has its own mask, 3.
DEFTYPE .l

!TestCase{mutex}
  GTMenuTitle 0,0,"Project"
  GTMenuItem 0,0,0,0,"Mx","M",6
  GTMenuItem 0,0,0,1,"Data","D",3,$1234
  t=Peek.l(Addr GTMenuList(0)+4)
  i=Peek.l(t+8)
  m1=Peek.l(i+22)
  i=Peek.l(i)
  m2=Peek.l(i+22)
  d=Peek.l(i+26)
  !CheckEq{m1,6,"mx"}
  !CheckEq{m2,3,"data_mask"}
  !CheckEq{d,$1234,"data_userdata"}
!EndTestCase

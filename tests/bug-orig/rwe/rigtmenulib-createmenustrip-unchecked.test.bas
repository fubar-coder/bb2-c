; rigtmenulib-createmenustrip-unchecked (#689) - CreateMenuStrip builds
; the NewMenu array from the object's lists without checking anything:
; with no title in the list it takes the "title" at address 0 (its text
; is the long at 4, ExecBase; its items the list at the long at 8) and
; walks whatever it finds there; CreateMenusA and GetVisualInfoA are
; not checked and the result is -1 in every case.  Compiled without the
; debugger (whose check would report the empty list).  First on a list
; that never had a title, then on a list whose only title GTFreeMenu
; removed after a strip was made of it: the old strip is freed, and the
; one made from address 0 is what GTSetMenu attaches (the program reads
; whether the object has a strip, +0, and whether the window has one,
; its MenuStrip at +28).
; Correct: CreateMenuStrip on a list without titles returns 0 and leaves
; the object without a strip, also after a strip was made before (that
; one is freed); a later GTSetMenu attaches nothing.
DEFTYPE .l

!TestCase{strip}
  WbToScreen 0
  Window 0,20,30,200,80,$000E,"Menus",1,2
  DefaultOutput
  r0=CreateMenuStrip(0)
  !CheckEq{r0,0,"no_title"}
  GTMenuTitle 1,0,"Project"
  GTMenuItem 1,0,0,0,"Open"
  r1=CreateMenuStrip(1)
  !CheckEq{r1,-1,"with_title"}
  GTFreeMenu 1,0
  r2=CreateMenuStrip(1)
  !CheckEq{r2,0,"after_free"}
  !CheckEq{Peek.l(Addr GTMenuList(1))<>0,0,"no_strip"}
  GTSetMenu 1
  !CheckEq{Peek.l(Peek.l(Addr Window(0))+28)<>0,0,"no_menu"}
!EndTestCase

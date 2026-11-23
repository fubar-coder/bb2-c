; rigtmenulib-freemenu-negative-item (#1089) - GTFreeMenu list#,menu,item
; treats a negative item number as "no item given" and frees the title
; through the item path: as an item of 30 bytes (the title node has 12),
; with the "shortcut" and the "sub items" read behind the node.  GTFreeMenu
; has no error check.  Here with runtime error checking and the item -1.
; Correct: the error check refuses the item number: "Illegal Menu Item
; number".
DEFTYPE .l

!TestCase{negative_item}
  GTMenuTitle 0,0,"Project"
  GTMenuItem 0,0,0,0,"A"
  !ExpectError{"Illegal Menu Item number","refused"}
  GTFreeMenu 0,0,-1
  !Check{0,"not_refused"}
!EndTestCase

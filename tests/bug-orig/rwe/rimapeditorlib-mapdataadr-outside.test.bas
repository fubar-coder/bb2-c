; rimapeditorlib-mapdataadr-outside (#892) - MapDataAdr(x,y) computes
; the address of the map word at x,y (inline: the data + y * width * 2 +
; x * 2) without a range check: x = 8 on a map 8 wide is the first word
; of the next row, a larger x or y lies behind the map.  A position the
; programmer gives; the original has no error check area.  Here with
; runtime error checking.
; Correct: MapDataAdr(7,7) works; the error check refuses x = 8: "Map
; position outside the map".
DEFTYPE .l

!TestCase{x_8}
  CreateMap 0,8,8,16,16,"m"
  a=MapDataAdr(7,7)
  ok=0
  If a=MapDataAdr+126 Then ok=1
  !CheckEq{ok,1,"inside"}
  !ExpectError{"Map position outside the map","refused"}
  a=MapDataAdr(8,0)
  !Check{0,"not_refused"}
!EndTestCase

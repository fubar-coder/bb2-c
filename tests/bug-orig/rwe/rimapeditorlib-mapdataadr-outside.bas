; rimapeditorlib-mapdataadr-outside (#892) - MapDataAdr(x,y) computes
; the address of the map word at x,y (inline: the data + y * width * 2 +
; x * 2) without a range check: x = 8 on a map 8 wide is the first word
; of the next row, a larger x or y lies behind the map.  A position the
; programmer gives; the original has no error check area.  Here with
; runtime error checking.
; Correct: MapDataAdr(7,7) works; the error check refuses x = 8: "Map
; position outside the map".
; @fixed #892
; @rwe rimapeditorlib
; @pass runtime_errors=on
DEFTYPE .l
CreateMap 0,8,8,16,16,"m"
a=MapDataAdr(7,7)
If a=MapDataAdr+126 Then NPrint "inside done"
a=MapDataAdr(8,0)
If a=MapDataAdr+16 Then NPrint "address of the next row"
End

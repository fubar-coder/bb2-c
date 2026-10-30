; rimapeditorlib-candraw-negative-xy (#892) - MapCanDraw refuses x/y
; outside the map with signed compares from above only (CMP.w : BGE):
; x = -1 passes and is used unsigned (65535): the address of the map word
; (kept for MapCanDrawAddr) lies 131070 bytes behind the map data (the
; word there is only read).
; Correct: x = -1 is refused, no address is kept (MapCanDrawAddr 0).
; @rwe rimapeditorlib
DEFTYPE .l
CreateMap 0,64,64,16,16,"level"
r=MapCanDraw(3,-1,0)
a=MapCanDrawAddr
If a=0 Then NPrint "no address"
If a=MapDataAdr+131070 Then NPrint "address behind the map"
End

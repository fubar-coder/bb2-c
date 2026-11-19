; rimapeditorlib-mapdatagrab-no-data (#895) - MapDataGrab writes the
; width * height values into the map's data without looking whether
; there are any.  A CreateMap that cannot allocate the data (here: width
; 0) leaves the map with data 0 and width and height 0, which the DBRA
; loops take as 65536 each: the grab writes from address 0 over all
; memory.  Made safe: after the failed CreateMap the map gets width and
; height 1 by hand, so one value is written - at address 0, whose long
; holds a marker (put back at the end).
; Correct: a map without data gets nothing (the marker stays).
; @rwe rimapeditorlib
DEFTYPE .l
b=AllocMem_(2,$10000)
If b=0 Then NPrint "no memory" : End
Poke.b b,7 : Poke.b b+1,8
CreateMap 0,0,1,16,16,"m"
If MapDataAdr<>0 Then NPrint "map has data" : End
m=Addr MapObject(0)
Poke.w m,1 : Poke.w m+2,1
o=Peek.l(0)
Poke.l 0,$12345678
MapDataGrab b,0,0
v=Peek.l(0)
Poke.l 0,o
Poke.w m,0 : Poke.w m+2,0
FreeMem_ b,2
If v=$12345678 Then NPrint "address 0 kept" Else NPrint "address 0 written"
End

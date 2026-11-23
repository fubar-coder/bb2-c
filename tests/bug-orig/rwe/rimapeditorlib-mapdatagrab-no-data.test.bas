; rimapeditorlib-mapdatagrab-no-data (#895) - MapDataGrab writes the
; width * height values into the map's data without looking whether
; there are any.  A CreateMap that cannot allocate the data (here: width
; 0) leaves the map with data 0 and width and height 0, which the DBRA
; loops take as 65536 each: the grab writes from address 0 over all
; memory.  Made safe: after the failed CreateMap the map gets width and
; height 1 by hand, so one value is written - at address 0, whose long
; holds a marker (put back at the end).
; Correct: a map without data gets nothing (the marker stays).
DEFTYPE .l

!TestCase{no_data}
  b=AllocMem_(2,$10000)
  !Require{b<>0,"allocated"}
  Poke.b b,7 : Poke.b b+1,8
  CreateMap 0,0,1,16,16,"m"
  !Require{MapDataAdr=0,"map_without_data"}
  mo=Addr MapObject(0)
  Poke.w mo,1 : Poke.w mo+2,1
  o=Peek.l(0)
  Poke.l 0,$12345678
  MapDataGrab b,0,0
  v=Peek.l(0)
  Poke.l 0,o
  Poke.w mo,0 : Poke.w mo+2,0
  FreeMem_ b,2
  !CheckEq{v,$12345678,"address_0_kept"}
!EndTestCase

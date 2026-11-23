; ribbmaplib-setbbmaploc-before-check (#554) - SetBBMapLoc stores the
; address before it checks for FORM BMAP: a failed call returns 0 but
; leaves the new address with the old map's end, info chunk and cache.
; Before any good call the info functions then read from address 0
; (BBMapWidth: the upper word of ExecBase); after a good one the chunk
; searches run from the rejected address up to the old map's end.  The
; map (built in memory) is FORM BMAP with MFRM (id 7, 40 x 30), MDAT and
; STRT; the rejected file at +64 is a FORM SCEN.
; Correct: a failed call changes nothing - before any good call the info
; functions give 0, after a good one the old map stays whole (STRT at
; 52, MDAT at 40).
DEFTYPE .l

!TestCase{failed_call}
  m=AllocMem_(128,$10001)
  !Require{m<>0,"memory"}
  Poke.l m,$464F524D : Poke.l m+4,48 : Poke.l m+8,$424D4150
  Poke.l m+12,$4D46524D : Poke.l m+16,12 : Poke.l m+20,7 : Poke.w m+24,40 : Poke.w m+26,30
  Poke.l m+32,$4D444154 : Poke.l m+36,4
  Poke.l m+44,$53545254 : Poke.l m+48,4
  Poke.l m+64,$464F524D : Poke.l m+68,4 : Poke.l m+72,$5343454E
  !CheckEq{SetBBMapLoc(m+64),0,"failed_first"}
  !CheckEq{BBMapWidth,0,"width_before"}
  !CheckEq{BBMapScenarioID,0,"id_before"}
  !CheckEq{SetBBMapLoc(m),-1,"good"}
  r=BBMapData : If r Then r=r-m
  !CheckEq{r,40,"data_good"}
  !CheckEq{SetBBMapLoc(m+64),0,"failed_second"}
  !CheckEq{BBMapWidth,40,"width_after"}
  !CheckEq{BBMapHeight,30,"height_after"}
  !CheckEq{BBMapScenarioID,7,"id_after"}
  r=BBMapData : If r Then r=r-m
  !CheckEq{r,40,"data_after"}
  r=BBMapStartPos : If r Then r=r-m
  !CheckEq{r,52,"start"}
  FreeMem_ m,128
!EndTestCase

; ribbmaplib-missing-mfrm (#551) - SetBBMapLoc stores the address the
; search for the MFRM chunk ends at as the info chunk, also when there is
; no MFRM: then it is the address behind the last chunk id read, behind
; the file, and BBMapInfo, BBMapScenarioID, BBMapWidth and BBMapHeight
; use the bytes there.  The map (built in memory) is FORM BMAP with only
; an MDAT chunk (24 bytes in all); behind it lie a zero long and the
; bytes a map info chunk would have (id 5, width 99, height 77).
; Correct: SetBBMapLoc succeeds (the other chunks are usable), the four
; info functions give 0, BBMapData finds MDAT (at 20).
; @fixed #551
; @rwe ribbmaplib
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,16 : Poke.l m+8,$424D4150
Poke.l m+12,$4D444154 : Poke.l m+16,4 : Poke.l m+20,$01020304
Poke.l m+28,5 : Poke.w m+32,99 : Poke.w m+34,77
NPrint SetBBMapLoc(m)
r=BBMapInfo : If r Then r=r-m
NPrint "info ",r
NPrint "id ",BBMapScenarioID
NPrint "width ",BBMapWidth
NPrint "height ",BBMapHeight
r=BBMapData : If r Then r=r-m
NPrint "data ",r
FreeMem_ m,64
End

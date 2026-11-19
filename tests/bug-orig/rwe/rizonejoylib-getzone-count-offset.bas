; rizonejoylib-getzone-count-offset (#870) - GetZoneX1/Y1/X2/Y2 compare the
; zone count of the table with the offset of the field in the zone (0, 2,
; 4, 6) instead of the zone number: in a table of 2 zones X2, Y1 and Y2
; return -1 for every zone, and a zone behind the table is read.  The
; zone behind table 1 lies in the 4 bytes that AllocMem adds to the 20
; of the table (blocks are multiples of 8 bytes); the program writes 1234
; there.
; Correct: every field of zone 1 (3 5 4 6), -1 for zone 2, which is not
; in the table.
; @rwe rizonejoylib
DEFTYPE .l
r=NewZoneTable(1,2)
UseZoneTable 1
ZoneInit
SetZone 1,3,4,5,6
NPrint GetZoneX1(1)," ",GetZoneX2(1)," ",GetZoneY1(1)," ",GetZoneY2(1)
Poke.w ZoneTable+20,1234
NPrint GetZoneX1(2)
End

; rizonejoylib-negative-zone-number (#873) - ZoneInit zone (and SetZone,
; ZoneInit start,end) check the zone number only against the upper bound:
; zone -1 is the 8 bytes in front of zone 0, the 4 bytes in front of the
; table block and the table's count.  ZoneInit -1 sets the count of table
; 0 (256 zones) to -1.  The count is put back before the end (finit frees
; the table with a size computed from it).
; Correct: zone -1 is refused, the table keeps its 256 zones.
; @rwe rizonejoylib
DEFTYPE .l
ZoneInit -1
n=ZoneTableSize
Poke.l ZoneTable,256
NPrint n
End

; rizonejoylib-negative-zone-number (#873) - ZoneInit zone (and SetZone,
; ZoneInit start,end) check the zone number only against the upper bound:
; zone -1 is the 8 bytes in front of zone 0, the 4 bytes in front of the
; table block and the table's count.  ZoneInit -1 sets the count of table
; 0 (256 zones) to -1.  Both longs are put back right after (under
; Forbid): finit frees the table with a size computed from the count, and
; the long in front of the block belongs to whatever memory lies there
; (another allocation, a free chunk) - left at -1 it crashed or hung some
; sessions and not others.
; Correct: zone -1 is refused, the table keeps its 256 zones.
; @rwe rizonejoylib
DEFTYPE .l
t=ZoneTable
Forbid_
o=Peek.l(t-4)
ZoneInit -1
n=ZoneTableSize
Poke.l t-4,o
Poke.l t,256
Permit_
NPrint n
End

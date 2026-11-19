; rizonejoylib-newzonetable-inits-current (#875) - NewZoneTable clears the
; zones of the current table (ZoneInit on _zonetable), not those of the
; new one, which keeps what AllocMem's block held.  Zone 0 of table 0 is
; set; then the program fills a block of the new table's size (300 zones,
; 2404 bytes) with zones 0..1000 x 0..1000 and frees it, and NewZoneTable
; (under Forbid, so nothing else allocates) gets that block back: table 0
; is cleared, and zone 1 of table 3 contains the point (15,15).
; Correct: table 0 keeps zone 0, all zones of table 3 are cleared.
; @rwe rizonejoylib
DEFTYPE .l
SetZone 0,10,10,20,20
Forbid_
a=AllocMem_(2404,0)
If a=0 Then Permit_ : NPrint "no memory" : End
For i=4 To 2396 Step 8
  Poke.w a+i,0 : Poke.w a+i+2,1000 : Poke.w a+i+4,0 : Poke.w a+i+6,1000
Next
FreeMem_ a,2404
r=NewZoneTable(3,300)
Permit_
NPrint "table 0: ",Zone(15,15)
UseZoneTable 3
NPrint "table 3: ",ZoneTest(1,15,15)
End

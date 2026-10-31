; riapplib-finit-slot-offset (#524) - at program end delapp clears the
; table slot of each object left with MOVE.l #0,0(a2,d7.l) where d7 is the
; slot index, not index * 4: for slot 1 an odd address, an address error
; on a 68000 (the session ends in a timeout).  The machine is the A600
; (68000).  The sessions run no Workbench, so AddAppMenu fails; AddAppIcon
; keeps the DiskObject in its slot also when AddAppIconA fails (#517):
; the DiskObject of slot 1 is left to finit.
; Correct: the program ends normally.
; @rwe riapplib
; @machine a600
; @pass timeout=30
DEFTYPE .l
r=AddAppIcon(1,"bug-orig","BB2:BLITZ2")
NPrint "icon added"
End

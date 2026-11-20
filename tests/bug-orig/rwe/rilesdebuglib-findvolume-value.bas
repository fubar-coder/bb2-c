; rilesdebuglib-findvolume-value (#1167) - D_FindVolume returns the DOS
; list entry it found, after it has unlocked the list again (below V36:
; after Permit): an address into a list the program no longer holds.
; Correct: as riamosfunclib's FindVolume, -1 when the name is there, 0
; when it is not, on Kickstart 1.x (a500) as from V36 on.
; @fixed #1167
; @rwe rilesdebuglib
; @machine a1200 a500
DEFTYPE .l
v$="RAM"
r=D_FindVolume(&v$)
If r=-1 Then NPrint "RAM: -1"
If r=0 Then NPrint "RAM: 0"
If r<>0 AND r<>-1 Then NPrint "RAM: an address"
v$="RWENOSUCHVOLUME"
NPrint "missing: ",D_FindVolume(&v$)
End

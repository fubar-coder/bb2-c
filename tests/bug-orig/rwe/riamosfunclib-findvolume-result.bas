; riamosfunclib-findvolume-result (#1082) - FindVolume is
; documented (RIAmosFuncLib guide) to return 0 if the volume is not
; present and -1 if it is.  It returns what FindDosEntry gave instead:
; the address of the DosList entry, taken after UnLockDosList (an entry
; of a list the program no longer holds).  As a truth value it works; a
; program that compares the result with -1 never finds the volume.
; Correct: FindVolume of RAM (always mounted) is -1, of a missing name 0.
; @rwe riamosfunclib
; @pass timeout=30
DEFTYPE .l
v$="RAM"
r=FindVolume(&v$)
If r=-1 Then NPrint "RAM: -1" Else If r=0 Then NPrint "RAM: 0" Else NPrint "RAM: other"
v$="RWENOSUCHVOLUME"
NPrint "missing: ",FindVolume(&v$)
End

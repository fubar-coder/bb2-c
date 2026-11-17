; riamosfunclib-saveexec-result (#501) - SaveExec returns -1 on success
; and 0 if the array is no list.  If the file cannot be created it
; returns the file name's address (a bare RTS with d0 as it is).  An
; empty list is not seen as empty: its head points to the list's tail
; node, which is written as a hunk, and the file gets a header for one
; hunk.
; Correct: 0 for the file that cannot be made, 0 and no file for the
; empty list.
; @fixed #501
; @rwe riamosfunclib
DEFTYPE .l
NEWTYPE .hunk
hunktype.w
hunkloc.l
hunklen.l
reloc.l
reloclen.l
flags.l
End NEWTYPE
Dim List h.hunk(4)
r=SaveExec("RAM:riamosfunclib-no-dir/x",h())
If r=0 Then NPrint "no file: 0" Else If r=-1 Then NPrint "no file: -1" Else NPrint "no file: other"
r=SaveExec("RAM:riamosfunclib-empty.exe",h())
NPrint "empty list: ",r
NPrint "file size: ",FileSize("RAM:riamosfunclib-empty.exe")
End

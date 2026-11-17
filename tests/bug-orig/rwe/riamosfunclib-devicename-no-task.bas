; riamosfunclib-devicename-no-task (#494) - DeviceName$ reads the name of
; the device's handler task through dol_Task -> mp_SigTask -> ln_Name
; without a check: a DosList entry without a handler (dol_Task 0) makes
; it read the long at address 16 as the task and copy whatever string
; that leads to.  The program adds a device entry of its own without a
; handler (MakeDosEntry/AddDosEntry) and removes it again.
; Correct: DeviceName$ returns "" for it.
; @fixed #494
; @rwe riamosfunclib
DEFTYPE .l
d=MakeDosEntry_("BBNOTASK",0)
If d=0 Then NPrint "no entry" : End
r=AddDosEntry_(d)
a$=DeviceName$("BBNOTASK")
If Len(a$)=0 Then NPrint "empty" Else NPrint "a name"
l=LockDosList_(6)
r=RemDosEntry_(d)
UnLockDosList_ 6
FreeDosEntry_ d
End

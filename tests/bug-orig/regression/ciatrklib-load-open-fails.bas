; ciatrklib-load-open-fails - LoadTrackerModule of a name that can be
; locked and examined but not opened (a directory, RAM:ciadir): the open
; by name fails (#1146 made the open by name and checks it).  On the
; a1200 the original's OpenFromLock gives the same results, so this is a
; test of the port's path, not of an original bug.
; Correct: the slot is freed, the result is not -1 (IoErr), and no lock
; is left on the directory: an exclusive lock on it works.
; @rwe ciatrklib
; @machine a500 a1200
; @pass timeout=30
DEFTYPE .l
k=CreateDir_("RAM:ciadir")
If k=0 Then NPrint "no directory" : End
UnLock_ k
r=LoadTrackerModule(0,"RAM:ciadir")
NPrint "loaded ",r=-1
NPrint "size ",GetTrackerSize(0)
l=Lock_("RAM:ciadir",-1)
NPrint "exclusive lock ",l<>0
If l Then UnLock_ l
End

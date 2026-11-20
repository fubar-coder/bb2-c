; ciatrklib-v36-calls (#1146) - LoadTrackerModule locks the file,
; examines it and opens it with OpenFromLock, a dos.library V36 function,
; on the program's dos.library without checking its version; on
; Kickstart 1.3 (dos V34) the offset lies behind dos.library's jump
; table: the program crashes at LoadTrackerModule (the session ends in a
; timeout; the recorded original shows none of the output before it, not
; even "before").  The module is the one of the test of #436.
; Correct: on every Kickstart LoadTrackerModule opens the file by its
; name while it holds the lock and unlocks the lock after that; the
; module is loaded, and the program ends normally.
; @fixed #1146
; @rwe ciatrklib
; @machine a500 a500-204 a1200
; @pass file=n.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
; @pass timeout=30
DEFTYPE .l
n$="CORPUS:n.mod"
NPrint "before"
r=LoadTrackerModule(0,n$)
NPrint "loaded ",r
NPrint "size ",GetTrackerSize(0)
FreeTrackerModule 0
NPrint "after"
End

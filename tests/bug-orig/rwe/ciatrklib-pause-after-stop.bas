; ciatrklib-pause-after-stop - PauseModule toggles its own state (#1076)
; (pause_status) and never looks whether a module plays: the first call
; stops the module (mt_end), the next enables playing again.  A
; StopTracker or FreeTrackerModule in between does not end the pause:
; the next PauseModule plays the stopped module again, or the module of
; the freed slot.  Without any pause, two PauseModule after a
; StopTracker also play the stopped module again.  The module (the one
; of the test of #436: speed 1, a song position per 64 ticks) is loaded
; into slot 1 and linked into slot 0 with DecodeModule (chip memory:
; FreeTrackerModule 0 clears the slot and frees no memory, so the
; original plays a module that is still there).  It is played from slot
; 0 without the CIA interrupt (ChangeTrackerVBlank, PlayTracker); each
; case compares the song position before and after 100 ticks, in this
; order:
;   "free":       StartTracker, PauseModule, FreeTrackerModule,
;                 PauseModule (then DecodeModule again);
;   "stop twice": StartTracker, StopTracker, PauseModule, PauseModule;
;   "stop":       StartTracker, PauseModule, StopTracker, PauseModule;
;   "again":      one more PauseModule.
; The original's state goes from case to case: it plays in "free",
; "stop twice" and "stop", and its last call pauses in "again".
; Correct: PauseModule pauses only a module that plays and continues
; only one it paused itself; StopTracker and FreeTrackerModule end the
; pause.  In every case the song position stays.
; @rwe ciatrklib
; @pass file=p.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
DEFTYPE .l
r=LoadTrackerModule(1,"CORPUS:p.mod")
NPrint "loaded ",r
If r=0 Then End
DecodeModule 0,GetTrackerLocation(1)
ChangeTrackerVBlank
r=StartTracker(0)
PauseModule
FreeTrackerModule 0
PauseModule
l0=GetModulePosition
For i=1 To 100 : PlayTracker : Next
NPrint "free ",l0=GetModulePosition
StopTracker
DecodeModule 0,GetTrackerLocation(1)
r=StartTracker(0)
StopTracker
PauseModule
PauseModule
l0=GetModulePosition
For i=1 To 100 : PlayTracker : Next
NPrint "stop twice ",l0=GetModulePosition
r=StartTracker(0)
PauseModule
StopTracker
PauseModule
l0=GetModulePosition
For i=1 To 100 : PlayTracker : Next
NPrint "stop ",l0=GetModulePosition
PauseModule
l0=GetModulePosition
For i=1 To 100 : PlayTracker : Next
NPrint "again ",l0=GetModulePosition
StopTracker
FreeTrackerModule 0
FreeTrackerModule 1
End

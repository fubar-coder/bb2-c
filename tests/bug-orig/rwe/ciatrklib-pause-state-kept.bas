; ciatrklib-pause-state-kept - PauseModule toggles its own state (#1076)
; (pause_status): the first call stops the module (mt_end), the next
; enables playing again.  StartTracker and StopTracker leave that state as
; it is: after a PauseModule, a StartTracker plays the module, but the
; next PauseModule takes it for "paused" and only enables playing - the
; module goes on (after a StopTracker the same call starts the stopped
; module again).  The module (the one of the test of #436: speed 1, a
; song position per 64 ticks) is played without the CIA interrupt
; (ChangeTrackerVBlank, PlayTracker): StartTracker, PauseModule,
; StartTracker, PauseModule, then 100 ticks.
; Correct: the second PauseModule pauses the module: the song position
; stays.
; @fixed #1076
; @rwe ciatrklib
; @pass file=p.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:p.mod")
NPrint "loaded ",r
If r=0 Then End
ChangeTrackerVBlank
r=StartTracker(0)
PauseModule
r=StartTracker(0)
PauseModule
l0=GetModulePosition
For i=1 To 100 : PlayTracker : Next
NPrint "paused ",l0=GetModulePosition
StopTracker
FreeTrackerModule 0
End

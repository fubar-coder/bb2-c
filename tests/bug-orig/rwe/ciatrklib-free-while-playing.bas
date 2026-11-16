; ciatrklib-free-while-playing (#436) - FreeTrackerModule frees the slot's
; module without looking whether it is playing: the CIA interrupt goes on
; playing the freed memory.  Freeing the module that plays is a programming
; error (the documentation of LoadTrackerModule: "The results are
; unpredictable"), and the original has no error check for it, not even
; with runtime error checking.  The module (generated: speed 1, no notes)
; is started without the CIA interrupt (ChangeTrackerVBlank; the program
; plays the ticks itself with PlayTracker); it compares the song position
; (GetModulePosition) before and after 100 ticks (speed 1: a position per
; 64 ticks), frees the module and compares the position again after 64
; more ticks.
; Here with runtime error checking.
; Correct: the error check refuses to free the module that plays.
; @fixed #436
; @rwe ciatrklib
; @pass file=f.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
; @pass runtime_errors=on
DEFTYPE .l
r=LoadTrackerModule(0,"CORPUS:f.mod")
NPrint "loaded ",r
If r
  ChangeTrackerVBlank
  r=StartTracker(0)
  l0=GetModulePosition
  For i=1 To 100 : PlayTracker : Next
  NPrint "playing before ",l0<>GetModulePosition
  FreeTrackerModule 0
  l1=GetModulePosition
  For i=1 To 64 : PlayTracker : Next
  l2=GetModulePosition
  StopTracker
  NPrint "playing ",l1<>l2
EndIf
End

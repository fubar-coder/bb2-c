; ciatrklib-free-while-playing (#436) - FreeTrackerModule frees the slot's
; module without looking whether it is playing: the CIA interrupt goes on
; playing the freed memory.  The module (generated: speed 1, no notes) is
; started without the CIA interrupt (ChangeTrackerVBlank; the program
; plays the ticks itself with PlayTracker - the interrupt does not play
; in the emulator); it compares the song position (GetModulePosition)
; before and after 100 ticks (speed 1: a position per 64 ticks), frees
; the module and compares the position again after 64 more ticks.
; Correct: freeing the playing module stops it - the position stays.
; @rwe ciatrklib
; @pass file=f.mod|@TESTDATA@/ciatrklib-free-while-playing.mod
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

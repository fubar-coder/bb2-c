; pt23a-arpeggio-overrun-ciatrklib (#474) - the arpeggio (0xy) finds the
; voice's period in the row of its finetune and plays the entry x (or y)
; half-tones higher without a limit: for a note near the top of the row
; the entry lies behind it, in the next finetune's row (a low note; for
; finetune -1 behind the table, voice 0's data).  Tested with ciatrklib
; (ritrackerlib has the same code; its registers are only seen by the
; library test).  The module (generated, loaded with LoadTrackerModule)
; plays period 127 (index 33 of the row, two below the top) with 047 in
; row 0.  ChangeTrackerMask 14 masks voice 0: its registers go to the
; play routine's fake_channel, which the program finds in its own code
; (the library is linked behind it: the period table, the four voices of
; 44 bytes and fake_channel behind it).  It reads fake_channel's period
; (+6) on the first tick after row 0 (the arpeggio's x).  The ticks are
; played by the program (ChangeTrackerVBlank, PlayTracker: speed 6, row
; 0 at the 6th tick; the CIA interrupt does not play in the emulator).
; Correct: four half-tones above index 33 is behind the row - the
; highest note of the row, 113.
; @fixed #474
; @rwe ciatrklib
; @pass file=pt.mod|@TESTDATA@/pt23a-arpeggio-overrun-ciatrklib.mod
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function
here:
r=LoadTrackerModule(0,"CORPUS:pt.mod")
If r=0 Then NPrint "no module" : End
ChangeTrackerVBlank
ChangeTrackerMask 14
r=StartTracker(0)
For i=1 To 7 : PlayTracker : Next
StopTracker
ChangeTrackerMask 15
t=findpt{?here}
NPrint "voices found ",t<>0
NPrint "arpeggio period ",Peek.w(t+1152+176+6)
FreeTrackerModule 0
End

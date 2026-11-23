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
; 0 at the 6th tick; the program counts the ticks itself instead of the
; CIA interrupt).
; Correct: four half-tones above index 33 is behind the row - the
; highest note of the row, 113.
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{arpeggio}
  here:
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,"CORPUS:pt.mod")
  !Require{r=-1,"loaded"}
  ChangeTrackerVBlank
  ChangeTrackerMask 14
  r=StartTracker(0)
  For i=1 To 7 : PlayTracker : Next
  StopTracker
  ChangeTrackerMask 15
  t=findpt{?here}
  !Require{t<>0,"voices_found"}
  !CheckEq{Peek.w(t+1152+176+6),113,"arpeggio_period"}
  FreeTrackerModule 0
!EndTestCase

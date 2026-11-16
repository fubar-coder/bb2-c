; pt23a-setperiod-entry37-ciatrklib (#481) - mt_SetPeriod looks the
; note's period up in finetune 0's row with 37 comparisons; a period
; below the table's lowest note 113 matches none and takes entry 37 of
; the voice's finetune row: the second entry of the next row, and for
; finetune -1 (15) the word behind the table, voice 0's n_cmd word.
; Tested with ciatrklib (pt23a-setperiod-entry37: ritrackerlib).  The
; module (generated, loaded with LoadTrackerModule) plays period 100 in
; row 0 on voice 0 (sample 1, finetune 0) and voice 1 (sample 2,
; finetune 15).  The program finds the play routine's data in its own
; code (the library is linked behind it: the period table, with the four
; voices of 44 bytes behind it) and reads the period (+16) of voices 0
; and 1 after row 0 was played.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: speed 6, row 1 at the 7th tick; the
; program counts the ticks itself instead of the CIA interrupt).
; Correct: the last entry of the row - 113 (finetune 0), 114 (-1).
; @fixed #481
; @rwe ciatrklib
; @pass file=pt.mod|@TESTDATA@/pt23a-setperiod-entry37-ciatrklib.mod
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
r=StartTracker(0)
For i=1 To 13 : PlayTracker : Next
StopTracker
t=findpt{?here}
NPrint "voices found ",t<>0
v=t+1152
NPrint "voice 0 period ",Peek.w(v+16)," voice 1 period ",Peek.w(v+44+16)
FreeTrackerModule 0
End

; medlib-1fxx-retrig (#205) - 1Fxy (note delay x, retrigger y) plays the
; delayed note in tick x with playfxnote and then takes the retrigger
; nibble from d1, which _PlayNote has changed (here: the length of the
; sample in words, 17): the nibble is 1, which divides every tick, so the
; note is struck a second time in the same tick.  The module here
; (generated) plays note 13 with 1F 30 (delay 3, no retrigger) in line 0
; of track 0, with a hold of 20 ticks.  Every strike by playfxnote adds
; the tick number to the hold counter (trk_noteoffcnt, +7 of the track),
; which also counts down by one per tick.  The program finds the track
; data of the player in its own code (the library is linked behind it;
; tracks 0-3, 98 bytes each, hold the address of their audio registers,
; $DFF0A0 + 16 * track, at +22) and reads the hold counter after tick 3
; (PlayMed plays one tick).
; Correct: one strike in tick 3 - the counter is 20 - 4 + 3 = 19 (a
; second strike adds 3 more).
; @configs clib
; @pass file=retrig.med|@TESTDATA@/medlib-1fxx-retrig.med
DEFTYPE .l
Function.l finddb{b.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=$DFF0A0
      If Peek.l(a+98)=$DFF0B0
        If Peek.l(a+196)=$DFF0C0 Then Function Return a
      EndIf
    EndIf
  Next
  Function Return 0
End Function
here:
x=finddb{?here}
NPrint "tracks found ",x<>0
If x=0 Then End
LoadMedModule 0,"CORPUS:retrig.med"
StartMedModule 0
For i=1 To 4 : PlayMed : Next
NPrint "hold counter ",Peek.b(x-22+7)
StopMed
End

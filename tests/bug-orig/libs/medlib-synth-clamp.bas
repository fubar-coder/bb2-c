; medlib-synth-clamp (#206) - at the end of the synth handling the period
; should be limited to at least 113, but the code sets d1 instead of d5
; (MOVEQ #113,d1): synth periods go below 113.  The module here
; (generated) plays a synth sound with period 202 whose waveform table
; changes the period by -100 per tick from the second tick on.  The Paula
; registers cannot be read back: the program finds the track data of the
; player in its own code (the library is linked behind it; tracks 0-3, 98
; bytes each, hold the address of their audio registers, $DFF0A0 + 16 *
; track, at +22) and points track 0 to a buffer, where the player then
; writes LC, LEN, PER (+6) and VOL.  PlayMed plays one tick; the program
; reads the period of the first three ticks.
; Correct: 202, 202, then 102 limited to 113.
; @configs clib
; @pass file=synth.med|@TESTDATA@/medlib-synth-clamp.med
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
Dim r.l(16)
x=finddb{?here}
NPrint "tracks found ",x<>0
If x=0 Then End
Poke.l x,&r(0)
LoadMedModule 0,"CORPUS:synth.med"
StartMedModule 0
Print "periods"
For i=1 To 3
  PlayMed
  Print " ",Peek.w(&r(0)+6)
Next
NPrint ""
StopMed
Poke.l x,$DFF0A0
End

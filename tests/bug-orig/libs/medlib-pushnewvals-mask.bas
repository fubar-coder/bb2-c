; medlib-pushnewvals-mask (#208) - after a note, PlayMed gives the
; channels their follow-up values (repeat part, or a zero word in chip
; memory for a sample without repeat) with pushnewvals, called once per
; bit set in ChanMask; but each call takes the next track and the next bit
; of the DMA mask: with SetMedMask %0101 it looks at tracks 0 and 1, and
; channel 2 never gets its follow-up values.  The module here (generated)
; plays a sample without repeat (64 bytes) in tracks 1 and 2 (track 2
; reads the note of the masked track 1, see the .spec).  The Paula
; registers cannot be read back: the program finds the track data of the
; player in its own code (the library is linked behind it; tracks 0-3, 98
; bytes each, hold the address of their audio registers, $DFF0A0 + 16 *
; track, at +22) and points track 2 to a buffer, where the player then
; writes LC (+0), LEN (+4), PER and VOL.  PlayMed plays one tick.
; Correct: channel 2 gets the follow-up values - LEN 1 (the zero word),
; LC no longer the sample.
; @configs clib
; @pass file=mask.med|@TESTDATA@/medlib-pushnewvals-mask.med
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
x=finddb{?here}+196
NPrint "tracks found ",x<>196
If x=196 Then End
Poke.l x,&r(0)
LoadMedModule 0,"CORPUS:mask.med"
d=Peek.l(Addr MedModule(0)+8)
StartMedModule 0
SetMedMask %0101
PlayMed
s=Peek.l(d+24)
NPrint "channel 2 LC at the sample ",Peek.l(&r(0))=Peek.l(s)+6," LEN ",Peek.w(&r(0)+4)
StopMed
Poke.l x,$DFF0C0
End

; medlib-tracks-4-15 (#209) - the structures of tracks 4-15 have 20 bytes
; (tracks 0-3: 98), but with SetMedMask bits 4-15 set _PlayNote handles a
; note of such a track like one of tracks 0-3: it writes trk_decay,
; trk_fadespd (+84/+85) and trk_vibroffs (+92) behind the structure and
; writes the sample to the "audio registers" at the address in +22 -
; bytes 2-5 of the next structure.  The module here (generated) has 8
; tracks and plays a note in track 4.  The program finds the track data of
; the player in its own code (the library is linked behind it; tracks
; 0-3, 98 bytes each, hold the address of their audio registers, $DFF0A0
; + 16 * track, at +22; tracks 4-7 and 8-15 follow with 20 bytes each).
; So that the write goes to no arbitrary address (bytes 2-5 of track 5
; are 0: address 0, ExecBase at 4), it points that address to a buffer;
; and it marks bytes 4-13 of track 8 (= +84..+93 of track 4).  PlayMed
; plays one tick.
; Correct: tracks 4-15 have no audio channel - the note writes nothing to
; the buffer and leaves track 8 alone.
; @configs clib
; @pass file=tracks.med|@TESTDATA@/medlib-tracks-4-15.med
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
t4=x-22+4*98
Poke.l t4+22,&r(0)
For i=84 To 93 : Poke.b t4+i,$55 : Next
LoadMedModule 0,"CORPUS:tracks.med"
StartMedModule 0
SetMedMask $FFFF
PlayMed
k=0
For i=84 To 93
  If Peek.b(t4+i)=$55 Then k=k+1
Next
NPrint "buffer written ",Peek.l(&r(0))<>0," track 8 kept ",k," of 10"
StopMed
Poke.l t4+22,0
End

; medlib-7xy-vibrato (#204) - tremolo 7xy with speed x = 0: in the first
; tick of the line plr_chgtrespd branches to the vibrato code (BEQ nonvib
; instead of nontre), which changes the period with the vibrato of the
; track.  The module here (generated) plays a note with vibrato 04 48 in
; line 0 and 07 04 in line 1 of track 0.  The Paula registers cannot be
; read back: the case finds the track data of the player in its own
; code (the library is linked behind it; tracks 0-3, 98 bytes each, hold
; the address of their audio registers, $DFF0A0 + 16 * track, at +22) and
; points track 0 to a buffer, where the player then writes LC, LEN, PER
; (+6) and VOL.  PlayMed plays one tick: 6 ticks per line, the seventh is
; the first tick of line 1; then the case compares the period written
; with the period of the note (trk_prevper, +20 of the track).
; Correct: tremolo leaves the period alone - it is the note's period.
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

!TestCase{period_kept}
  here:
  Dim r.l(16)
  x=finddb{?here}
  !Require{x<>0,"tracks_found"}
  Poke.l x,&r(0)
  LoadMedModule 0,"CORPUS:trem.med"
  StartMedModule 0
  For i=1 To 7 : PlayMed : Next
  same=0
  If Peek.w(&r(0)+6)=Peek.w(x-2) Then same=1
  StopMed
  Poke.l x,$DFF0A0
  !CheckEq{same,1,"period_of_the_note"}
!EndTestCase

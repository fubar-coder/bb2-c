; medlib-getmedvolume-clears (#212) - GetMedVolume reads trk_prevvol and
; clears it, but that is the volume the player writes to AUDxVOL in every
; tick: after GetMedVolume the channel is silent until the next note with
; an instrument.  The module here (generated) plays a note with
; instrument 1 (volume 64) in line 0 of track 0.  The case drives the
; player with PlayMed (one tick each).  The Paula registers cannot be read
; back: the case finds the track data of the player in its own code
; (the library is linked behind it; tracks 0-3, 98 bytes each, hold the
; address of their audio registers, $DFF0A0 + 16 * track, at +22) and
; points track 0 to a buffer, where the player then writes LC, LEN, PER
; and VOL (+9, a byte).
; Correct: GetMedVolume only reads - the next tick writes volume 64 again.
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

!TestCase{next_tick}
  here:
  Dim r.l(16)
  x=finddb{?here}
  !Require{x<>0,"tracks_found"}
  Poke.l x,&r(0)
  LoadMedModule 0,"CORPUS:vol.med"
  StartMedModule 0
  PlayMed
  v1=Peek.b(&r(0)+9)
  rd=GetMedVolume(0)
  PlayMed
  v2=Peek.b(&r(0)+9)
  StopMed
  Poke.l x,$DFF0A0
  !CheckEq{v1,64,"volume"}
  !CheckEq{rd,64,"read"}
  !CheckEq{v2,64,"next_tick"}
!EndTestCase

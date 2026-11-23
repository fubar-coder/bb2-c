; medlib-mask-noteptr (#334) - the player skips a track masked out by
; SetMedMask before it reads the track's note, and only reading a note
; moves the note pointer on: the next track reads the note of the masked
; track, every following track the note of the track before it.  The
; module here (generated) plays notes 13, 25, 37 in tracks 0-2 of line 0,
; track 3 is empty; track 0 is masked out.  PlayMed plays one tick,
; GetMedNote reads the note each track played.
; Correct: track 0 plays nothing, tracks 1 and 2 their own notes, track 3
; nothing.
DEFTYPE .l

!TestCase{masked_track_0}
  LoadMedModule 0,"CORPUS:mask.med"
  StartMedModule 0
  SetMedMask %1110
  PlayMed
  n0=GetMedNote(0)
  n1=GetMedNote(1)
  n2=GetMedNote(2)
  n3=GetMedNote(3)
  StopMed
  !CheckEq{n0,0,"track_0"}
  !CheckEq{n1,25,"track_1"}
  !CheckEq{n2,37,"track_2"}
  !CheckEq{n3,0,"track_3"}
!EndTestCase

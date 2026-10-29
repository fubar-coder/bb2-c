; medlib-mask-noteptr (#334) - the player skips a track masked out by
; SetMedMask before it reads the track's note, and only reading a note
; moves the note pointer on: the next track reads the note of the masked
; track, every following track the note of the track before it.  The
; module here (generated) plays notes 13, 25, 37 in tracks 0-2 of line 0,
; track 3 is empty; track 0 is masked out.  PlayMed plays one tick,
; GetMedNote reads the note each track played.
; Correct: track 0 plays nothing, tracks 1 and 2 their own notes, track 3
; nothing.
; @configs clib
; @pass file=mask.med|@TESTDATA@/medlib-mask-noteptr.med
LoadMedModule 0,"CORPUS:mask.med"
StartMedModule 0
SetMedMask %1110
PlayMed
NPrint "notes ",GetMedNote(0)," ",GetMedNote(1)," ",GetMedNote(2)," ",GetMedNote(3)
StopMed
End

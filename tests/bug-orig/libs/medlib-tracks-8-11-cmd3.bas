; medlib-tracks-8-11-cmd3 (#1067) - the line routine tests a track against the
; channel mask with BTST d7,ChanMask+1, a bit of the low byte (the track
; number mod 8): with the default mask 15 the tracks 8-11 pass like tracks
; 0-3.  Their structures have 20 bytes (tracks 0-3: 98), and the commands
; 03 (portamento) and FFD (period) have the track check only with #CHECK
; = 1: 03 writes trk_porttrgper (+32) and trk_prevportspd (+82) of such a
; track into the structures of the tracks behind it - for track 8 +82 is
; trk_prevvol (+2) of track 12.  The module here (generated) has 9 tracks
; and 03 20 (portamento speed $20, no note) in track 8 (and in track 4,
; whose note the original's track 8 reads, #334).  PlayMed plays one line
; with the default mask; GetMedVolume(12) reads trk_prevvol of track 12.
; Correct: tracks 4-15 have no channel, their commands change no other
; track: the volume of track 12 stays 0.
; @fixed #1067
; @configs clib
; @pass file=cmd3.med|@TESTDATA@/medlib-tracks-8-11-cmd3.med
DEFTYPE .l
LoadMedModule 0,"CORPUS:cmd3.med"
StartMedModule 0
NPrint "before ",GetMedVolume(12)
PlayMed
NPrint "track 12 volume ",GetMedVolume(12)
StopMed
End

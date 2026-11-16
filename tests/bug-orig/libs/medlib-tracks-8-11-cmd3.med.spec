; medlib-tracks-8-11-cmd3: a MED module (MMD1, 9 tracks), no samples, a
; song of one position, 6 ticks per line; block 0 has one line with 03 20
; (portamento speed $20, no note) in tracks 4 and 8 (the original's track
; 8 reads the note of track 4, #334; with that fixed it reads its own)
s "MMD1"
l @end                  ; module length
l @song
w 0 0                   ; psecnum, pseq
l @blockarr
b 0 0 0 0               ; flags, reserved
l 0                     ; sample array: none
l 0
l 0                     ; expansion data: none
l 0
w 0 0 0 0               ; pstate, pblock, pline, pseqnum
w 0                     ; actplayline
b 0 0                   ; counter, extra songs
song:
fill 504 0              ; 63 samples: none
w 1                     ; blocks
w 1                     ; song length
fill 256 0              ; play sequence: block 0
w 125                   ; tempo
b 0 0 0 6               ; transpose, flags, flags2, ticks per line
fill 16 64              ; track volumes
b 64 0                  ; master volume, samples
blockarr:
l @block0
block0:
w 9 0                   ; tracks, lines - 1
l 0                     ; block info: none
l 0 0 0 0               ; tracks 0-3: empty
b 0 0 $03 $20           ; track 4: 03 20
l 0 0 0                 ; tracks 5-7: empty
b 0 0 $03 $20           ; track 8: 03 20
end:

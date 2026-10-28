; medlib-bxx-word: a MED module (MMD1: the commands 19xx and up need its 4
; byte notes), no samples, a song of 8 positions (all block 0), 6 ticks
; per line; block 0 has 4 tracks and one line: track 0 19 08 (sample
; offset, no note), track 1 0B 05 (position jump to 5)
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
w 8                     ; song length
fill 256 0              ; play sequence: block 0
w 125                   ; tempo
b 0 0 0 6               ; transpose, flags, flags2, ticks per line
fill 16 64              ; track volumes
b 64 0                  ; master volume, samples
blockarr:
l @block0
block0:
w 4 0                   ; tracks, lines - 1
l 0                     ; block info: none
b 0 0 $19 $08           ; track 0: 19 08
b 0 0 $0b $05           ; track 1: 0B 05
l 0 0                   ; tracks 2, 3: empty
end:

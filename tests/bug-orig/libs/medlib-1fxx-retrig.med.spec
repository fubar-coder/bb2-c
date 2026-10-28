; medlib-1fxx-retrig: a MED module (MMD1), one sample (instrument 1: 34
; bytes = 17 words, volume 64, hold 20 from the InstrExt of the expansion
; data), a song of one position, 6 ticks per line; block 0 has 4 tracks
; and 4 lines, track 0 plays note 13 with instrument 1 and 1F 30 (note
; delay 3, no retrigger) in line 0
s "MMD1"
l @end                  ; module length
l @song
w 0 0                   ; psecnum, pseq
l @blockarr
b 0 0 0 0               ; flags, reserved
l @smplarr
l 0
l @exp                  ; expansion data
l 0
w 0 0 0 0               ; pstate, pblock, pline, pseqnum
w 0                     ; actplayline
b 0 0                   ; counter, extra songs
song:
w 0 0                   ; sample 1: repeat, repeat length
b 0 0 64 0              ;   MIDI channel, preset, volume, transpose
fill 496 0              ; samples 2-63: none
w 1                     ; blocks
w 1                     ; song length
fill 256 0              ; play sequence: block 0
w 125                   ; tempo
b 0 0 0 6               ; transpose, flags, flags2, ticks per line
fill 16 64              ; track volumes
b 64 1                  ; master volume, samples
blockarr:
l @block0
smplarr:
l @sample1
exp:
l 0                     ; next module: none
l @instrext
w 1 4                   ; InstrExt: entries, entry size
fill 56 0               ; the rest: no annotation, dumps, names ...
instrext:
b 20 0 0 0              ; instrument 1: hold 20, decay, MIDI, finetune
block0:
w 4 3                   ; tracks, lines - 1
l 0                     ; block info: none
b 13 1 $1f $30          ; line 0, track 0: note 13, instrument 1, 1F 30
l 0 0 0                 ; tracks 1-3
fill 48 0               ; lines 1-3
sample1:
l 34                    ; length
w 0                     ; type: sample
fill 34 0
end:

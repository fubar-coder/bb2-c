; medlib-synth-clamp: a MED module (MMD1), one synth instrument, a song of
; one position, 6 ticks per line; block 0 has 4 tracks and 4 lines, track
; 0 plays note 50 (period 202) with instrument 1 in line 0.  The synth
; sound: volume table 64, end; waveform table: waveform 0, period change
; up by 100 per waveform step (CHU, $F3 100), end; one waveform of 16
; words; both tables step every tick
s "MMD1"
l @end                  ; module length
l @song
w 0 0                   ; psecnum, pseq
l @blockarr
b 0 0 0 0               ; flags, reserved
l @smplarr
l 0
l 0                     ; expansion data: none
l 0
w 0 0 0 0               ; pstate, pblock, pline, pseqnum
w 0                     ; actplayline
b 0 0                   ; counter, extra songs
song:
w 0 0                   ; instrument 1: repeat, repeat length
b 0 0 64 0              ;   MIDI channel, preset, volume, transpose
fill 496 0              ; instruments 2-63: none
w 1                     ; blocks
w 1                     ; song length
fill 256 0              ; play sequence: block 0
w 125                   ; tempo
b 0 0 0 6               ; transpose, flags, flags2, ticks per line
fill 16 64              ; track volumes
b 64 1                  ; master volume, instruments
blockarr:
l @block0
smplarr:
l @synth
block0:
w 4 3                   ; tracks, lines - 1
l 0                     ; block info: none
b 50 1 0 0              ; line 0, track 0: note 50, instrument 1
l 0 0 0                 ; tracks 1-3
fill 48 0               ; lines 1-3
synth:
l 0                     ; length (not used)
w -1                    ; type: synth
b 0 0 0 0               ; default decay, reserved
w 0 0                   ; repeat, repeat length
w 2 4                   ; volume table, waveform table length
b 1 1                   ; volume speed, waveform speed
w 1                     ; waveforms
b 64 $ff                ; volume table: 64, end
fill 126 0
b 0 $f3 100 $ff         ; waveform table: waveform 0, CHU 100, end
fill 124 0
l @wf0-@synth           ; waveform 0 (relative to the instrument)
wf0:
w 16                    ; length in words
fill 32 0
end:

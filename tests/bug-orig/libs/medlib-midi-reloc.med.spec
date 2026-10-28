; medlib-midi-reloc: a MED module (MMD1), no samples, a song of one
; position, one empty block of 4 tracks and 1 line; expansion data with
; one MIDI dump (8 bytes) and no next module
s "MMD1"
l @end                  ; module length
l @song
w 0 0                   ; psecnum, pseq
l @blockarr
b 0 0 0 0               ; flags, reserved
l 0                     ; sample array: none
l 0
l @exp                  ; expansion data
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
w 4 0                   ; tracks, lines - 1
l 0                     ; block info: none
l 0 0 0 0               ; line 0: empty
exp:
l 0                     ; next module: none
l 0                     ; InstrExt: none
w 0 0                   ; entries, entry size
fill 40 0               ; annotation, InstrInfo, jump mask, rgb, ... names
l @dumps                ; +52: MIDI dumps
fill 16 0
dumps:
w 1                     ; number of dumps
w 0 0 0                 ; reserved
l @dump0
dump0:
l 8                     ; length
l @dumpdata             ; data
w 0                     ; ext_len
dumpdata:
b $f0 $7e $7f $09 $01 $f7 0 0
end:

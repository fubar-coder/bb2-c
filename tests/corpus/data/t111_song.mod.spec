; t111_song.mod: the module t111_tracker loads - a NoiseTracker module
; (M.K., 31 sample slots), three samples with data, song length 8,
; restart 127, two patterns (the highest pattern number 1 is first used
; at position 1); size 1084 + 2 * 1024 + 208 = 3340 bytes.
; A note: sample number high nibble + period (12 bits), sample number low
; nibble + effect, effect data; no effects here.
s "corpus t111"         ; name (20 bytes)
fill 9 0
s "ramp"                ; sample 1: 32 words, volume 64, no repeat
fill 18 0
w 32
b 0 64
w 0 1
s "square"              ; sample 2: 48 words, volume 64, no repeat
fill 16 0
w 48
b 0 64
w 0 1
s "saw"                 ; sample 3: 24 words, volume 64, no repeat
fill 19 0
w 24
b 0 64
w 0 1
fill 840 0              ; samples 4-31: empty (28 * 30 bytes)
b 8 127                 ; song length, restart
b 0 1 0 1 1 0 0 1       ; positions 0-7
fill 120 0              ; positions 8-127: pattern 0
s "M.K."
pattern0:
b $01 $AC $10 0  $01 $53 $20 0  $01 $1D $30 0  0 0 0 0     ; row 0
fill 48 0               ; rows 1-3
b 0 0 0 0  0 0 0 0  0 0 0 0  $00 $D6 $10 0                 ; row 4
fill 48 0               ; rows 5-7
b $01 $40 $20 0  0 0 0 0  $00 $FE $10 0  $01 $AC $30 0     ; row 8
fill 880 0              ; rows 9-63
pattern1:
b $01 $1D $30 0  $00 $D6 $10 0  0 0 0 0  $01 $53 $20 0     ; row 0
fill 112 0              ; rows 1-7
b 0 0 0 0  $01 $AC $20 0  $01 $40 $10 0  0 0 0 0           ; row 8
fill 880 0              ; rows 9-63
sample1:                ; 64 bytes: a ramp
b 0 4 8 12 16 20 24 28 32 36 40 44 48 52 56 60
b 64 68 72 76 80 84 88 92 96 100 104 108 112 116 120 124
b -128 -124 -120 -116 -112 -108 -104 -100 -96 -92 -88 -84 -80 -76 -72 -68
b -64 -60 -56 -52 -48 -44 -40 -36 -32 -28 -24 -20 -16 -12 -8 -4
sample2:                ; 96 bytes: a square wave
fill 24 100
fill 24 -100
fill 24 100
fill 24 -100
sample3:                ; 48 bytes: a saw tooth
b 120 105 90 75 60 45 30 15 0 -15 -30 -45 -60 -75 -90 -105
b 120 100 80 60 40 20 0 -20 -40 -60 -80 -100 -120 70 35 1
b 9 18 27 36 45 54 63 72 81 90 99 108 117 126 3 6

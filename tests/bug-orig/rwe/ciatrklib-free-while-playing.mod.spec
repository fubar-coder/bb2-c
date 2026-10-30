; ciatrklib-free-while-playing: a tracker module (M.K.), one pattern, no
; samples (2108 bytes); the song plays pattern 0 three times; row 0 sets
; speed 1 (F01: one row per frame, a position per 64 frames), the other
; rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; samples 1-31: empty
b 3 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $00000f01 0 0 0       ; row 0, channel 0: F01
fill 1008 0             ; rows 1-63

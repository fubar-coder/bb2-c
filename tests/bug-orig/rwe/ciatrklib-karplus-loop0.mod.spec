; ciatrklib-karplus-loop0: a tracker module (M.K.), one pattern, no
; samples (2108 bytes); row 0, channel 0 has E81 (Karplus-Strong) without
; a note, the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; samples 1-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $00000e81 0 0 0       ; row 0, channel 0: E81
fill 1008 0             ; rows 1-63

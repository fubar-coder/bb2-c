; trackerlib-samplenummer: a tracker module (M.K.), one pattern, no
; samples; row 0, channel 0 plays "sample 32" (period 428), the other rows
; are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; samples 1-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $21ac0000 0 0 0       ; row 0, channel 0: sample 32, period 428
fill 1008 0             ; rows 1-63
l 0                     ; where the (empty) samples begin

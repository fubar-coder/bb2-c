; trackerlib-c-f-vorzeichen: a tracker module (M.K.), one pattern, no
; samples.  Row 0, channel 0: F80 (speed $80); channel 1 carries a row
; marker as the parameter of the unused command 8: 0 in row 0, 1 in rows
; 1-15; the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; samples 1-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $00000f80 $00000800 0 0       ; row 0
l 0 $00000801 0 0       ; row 1
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0       ; row 5
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0       ; row 10
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0
l 0 $00000801 0 0       ; row 15
fill 768 0              ; rows 16-63
l 0                     ; where the (empty) samples begin

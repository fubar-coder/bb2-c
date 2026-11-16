; pt23a-arpeggio-overrun-ciatrklib: a tracker module (M.K.), one
; pattern; sample 1 (2 words, finetune 0).  Row 0 plays it on channel 0
; at period 127 with 047 (arpeggio), the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 22 0               ; sample 1: name
w 2                     ;   length in words
b 0 $40                 ;   finetune 0, volume
w 0 1                   ;   repeat start, repeat length (words)
fill 900 0              ; samples 2-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $007f1047 0 0 0       ; row 0, channel 0: sample 1, period 127, 047
fill 1008 0             ; rows 1-63
l 0                     ; sample 1

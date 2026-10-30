; pt23a-finetune-above-15: a tracker module (M.K.), one pattern; sample
; 31 (2 words) has the finetune byte 16.  Row 0 plays it on channel 0
; (period 428), the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 900 0              ; samples 1-30: empty
fill 22 0               ; sample 31: name
w 2                     ;   length in words
b 16 $40                ;   finetune 16, volume
w 0 1                   ;   repeat start, repeat length (words)
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $11acf000 0 0 0       ; row 0, channel 0: sample 31, period 428
fill 1008 0             ; rows 1-63
l 0                     ; sample 31

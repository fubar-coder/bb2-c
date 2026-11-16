; pt23a-setperiod-entry37: a tracker module (M.K.), one pattern; sample 1
; (2 words, finetune 0) and sample 2 (2 words, finetune 15).  Row 0 plays
; period 100 (below the table) with sample 1 on channel 0 and with sample
; 2 on channel 1, the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 22 0               ; sample 1: name
w 2                     ;   length in words
b 0 $40                 ;   finetune 0, volume
w 0 1                   ;   repeat start, repeat length (words)
fill 22 0               ; sample 2: name
w 2                     ;   length in words
b 15 $40                ;   finetune 15 (-1), volume
w 0 1                   ;   repeat start, repeat length (words)
fill 870 0              ; samples 3-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $00641000 $00642000 0 0  ; row 0: channel 0 sample 1, channel 1 sample 2, period 100
fill 1008 0             ; rows 1-63
l 0 0                   ; samples 1 and 2

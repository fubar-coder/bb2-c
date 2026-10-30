; pt23a-samplenum-over-31: a tracker module (M.K.), one pattern; sample 1
; has 2 words, samples 2-31 are empty.  Row 0 plays sample 1 on channel 0
; (period 428), row 1 "sample 33" (period 428), the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 22 0               ; sample 1: name
w 2                     ;   length in words
b 0 $40                 ;   finetune, volume
w 0 1                   ;   repeat start, repeat length (words)
fill 900 0              ; samples 2-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $01ac1000 0 0 0       ; row 0, channel 0: sample 1, period 428
l $21ac1000 0 0 0       ; row 1, channel 0: sample 33, period 428
fill 992 0              ; rows 2-63
l 0                     ; sample 1

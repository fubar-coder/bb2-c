; trackerlib-setvol-byte: a tracker module (M.K.), one pattern; sample 1
; has 4 bytes, volume $20.  Channel 0: row 0 plays sample 1 (period 428)
; with C10 (volume $10), row 1 slides the volume down (A01: 1 per frame,
; 5 frames at speed 6); the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 22 0               ; sample 1: name
w 2                     ;   length in words
b 0 $20                 ;   finetune, volume
w 0 1                   ;   repeat start, repeat length (words)
fill 900 0              ; samples 2-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $01ac1c10 0 0 0       ; row 0, channel 0: sample 1, period 428, C10
l $00000a01 0 0 0       ; row 1, channel 0: A01
fill 992 0              ; rows 2-63
l 0                     ; the data of sample 1

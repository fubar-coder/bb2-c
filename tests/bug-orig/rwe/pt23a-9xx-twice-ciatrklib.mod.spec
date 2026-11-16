; pt23a-9xx-twice-ciatrklib: a tracker module (M.K.), one pattern; sample 31
; claims 2000 words (no repeat); the module holds only its first long.
; Row 0 plays it on channel 0 (period 428) with 902 (offset 256 words),
; the other rows are empty
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 900 0              ; samples 1-30: empty
fill 22 0               ; sample 31: name
w 2000                  ;   length in words
b 0 $40                 ;   finetune, volume
w 0 1                   ;   repeat start, repeat length (words)
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
l $11acf902 0 0 0       ; row 0, channel 0: sample 31, period 428, 902
fill 1008 0             ; rows 1-63
l 0                     ; the start of sample 31

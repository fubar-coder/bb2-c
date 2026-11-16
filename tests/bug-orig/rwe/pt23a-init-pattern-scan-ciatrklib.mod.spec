; pt23a-init-pattern-scan-ciatrklib: a tracker module (M.K.), three patterns, no
; samples; song length 1 (pattern 0), but the position table names
; pattern 1 at position 126 (a new highest pattern there) and pattern 2
; at position 127.  The last note of pattern 0 holds the byte $7F (an
; effect parameter), the highest value a signed compare finds in the
; data behind the position table
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; samples 1-31: empty
b 1 0                   ; song length, restart
fill 126 0              ; positions 0-125: pattern 0
b 1 2                   ; positions 126, 127: patterns 1, 2
s "M.K."
fill 1020 0             ; pattern 0, rows 0-62 and channels 0-2 of row 63
l $0000007f             ; row 63, channel 3: $7F
fill 2048 0             ; patterns 1 and 2
l 0                     ; behind the patterns (cleared by mt_init)

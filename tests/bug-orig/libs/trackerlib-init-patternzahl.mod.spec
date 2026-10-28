; trackerlib-init-patternzahl: a tracker module (M.K., 31 samples) whose
; highest pattern number (1) is first used at position 126; all samples
; empty (length 0: their starts are all behind the highest pattern).
; Markers: the long at the start of pattern 2 ($11111111, where the samples
; begin) and at the start of "pattern 78" ($22222222, where they begin when
; the search reads on through the 64K behind the position table and takes
; the "M" of M.K. as the highest pattern number); the module is long enough
; for both.
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 930 0              ; 31 sample headers (30 bytes each): empty
b 127 0                 ; song length, restart
fill 126 0              ; positions 0-125: pattern 0
b 1 0                   ; position 126: pattern 1, position 127: 0
s "M.K."
fill 2048 0             ; patterns 0 and 1: empty rows
pattern2:
l $11111111
fill $13c3c-@pattern2-4 0     ; up to $43c + 78 * 1024
l $22222222
fill 1020 0

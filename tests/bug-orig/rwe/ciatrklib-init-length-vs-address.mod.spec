; ciatrklib-init-length-vs-address: a tracker module (M.K.), one pattern
; (empty), sample 1 of 4 words without a loop (2116 bytes); the sample's
; bytes are $11 $22 ... $88
s "bug-orig"            ; name (20 bytes)
fill 12 0
fill 22 0               ; sample 1: name
w 4                     ; length (words)
b 0 64                  ; finetune, volume
w 0 1                   ; repeat start, repeat length (no loop)
fill 900 0              ; samples 2-31: empty
b 1 0                   ; song length, restart
fill 128 0              ; positions: pattern 0
s "M.K."
fill 1024 0             ; pattern 0
b $11 $22 $33 $44 $55 $66 $77 $88   ; sample 1

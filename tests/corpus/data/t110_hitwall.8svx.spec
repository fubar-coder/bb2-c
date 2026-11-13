; t110_hitwall.8svx: the second sound t110_audio loads into the same
; object - a one-shot part (40 bytes) and a repeat part (24 bytes),
; 8363 samples per second (period 428)
form 8SVX
chunk VHDR
l 40 24 0               ; one shot, repeat, samples per cycle
w 8363                  ; samples per second
b 1 0                   ; octaves, no compression
l $10000                ; volume 1.0
end
chunk BODY
b 1 12 23 34 45 56 67 78 89 100             ; one-shot part: a triangle
b 111 122 127 116 105 94 83 72 61 50
b -11 -22 -33 -44 -55 -66 -77 -88 -99 -110
b -121 -128 -117 -106 -95 -84 -73 -62 -51 -40
b 64 -64 48 -48 32 -32 16 -16 8 -8 4 -4     ; repeat part
b 60 -60 44 -44 28 -28 12 -12 2 -2 1 -1
end
end

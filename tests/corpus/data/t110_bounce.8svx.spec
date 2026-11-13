; t110_bounce.8svx: the first sound t110_audio loads - one-shot only
; (24 bytes, no repeat part), 10000 samples per second (period 357)
form 8SVX
chunk VHDR
l 24 0 0                ; one shot, repeat, samples per cycle
w 10000                 ; samples per second
b 1 0                   ; octaves, no compression
l $10000                ; volume 1.0
end
chunk BODY
b 8 16 24 32 40 48 56 64 72 80 88 96        ; a ramp up
b 100 90 80 70 60 50 40 30 20 10 5 1        ; and down
end
end

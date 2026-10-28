; audiolib-diskplay-endless: an 8SVX with a BODY of 2048 bytes (twice the
; DiskBuffer of the test), silence, 8000 samples per second
form 8SVX
chunk VHDR
l 2048 0 0              ; one shot, repeat, samples per cycle
w 8000                  ; samples per second
b 1 0                   ; octaves, no compression
l $10000                ; volume 1.0
end
chunk BODY
fill 2048 0
end
end

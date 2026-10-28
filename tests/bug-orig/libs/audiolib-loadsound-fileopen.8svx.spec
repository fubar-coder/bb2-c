; audiolib-loadsound-fileopen: an 8SVX whose BODY claims 131072 bytes (the
; limit of LoadSound is below 128K); the data itself is not there - the
; length is checked before anything is read
form 8SVX
chunk VHDR
l 4 0 0                 ; one shot, repeat, samples per cycle
w 8000                  ; samples per second
b 1 0                   ; octaves, no compression
l $10000                ; volume 1.0
end
chunk BODY 131072
fill 16 0
end
end

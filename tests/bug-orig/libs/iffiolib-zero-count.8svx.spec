; iffiolib-zero-count: an 8SVX whose BODY is empty (length 0), followed by
; an empty ANNO chunk (8 bytes: the bytes iffread copies when it counts
; down from 2^32)
form 8SVX
chunk VHDR
l 0 0 0                 ; one shot, repeat, samples per cycle
w 8000                  ; samples per second
b 1 0                   ; octaves, no compression
l $10000                ; volume 1.0
end
chunk BODY
end
chunk ANNO
end
end

; shapeslib-anim-delta-bounds: an ANIM5 of 16 x 1 pixels, two planes;
; frame 0 an uncompressed ILBM (plane 0 $0102, plane 1 $0304), then one
; delta frame (interleave 1) whose plane 0 column 0 skips one row - the
; plane has only one - and sets the byte there to $EE
form ANIM
form ILBM
chunk BMHD
w 16 1                  ; width, height
w 0 0                   ; position
b 2 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk BODY
b $01 $02 $03 $04       ; row 0: plane 0, plane 1
end
end
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 1 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 1 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64                    ; plane 0
fill 60 0               ; planes 1-15: none
b 2 1 $81 $EE           ; column 0: skip 1 row, then 1 byte
b 0                     ; column 1
end
end
end

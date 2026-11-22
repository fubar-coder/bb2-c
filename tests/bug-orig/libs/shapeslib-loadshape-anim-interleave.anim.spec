; shapeslib-loadshape-anim-interleave: an ANIM5 of 16 x 2 pixels, one plane;
; frame 0 an uncompressed ILBM (rows $0102 and $0304), then 8 delta frames
; with the interleaves 0, 0, 0, 0, 1, 2, 3, 255 (each DLTA: 16 plane
; offsets, plane 0 at 64; per column the number of ops, then the ops:
; 1-127 skip rows, $80+n n bytes)
form ANIM
form ILBM
chunk BMHD
w 16 2                  ; width, height
w 0 0                   ; position
b 1 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk BODY
b $01 $02 $03 $04
end
end
; frame 1, interleave 0: from frame 0, row 0 column 0 = $10
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 1 $81 $10             ; column 0
b 0                     ; column 1
end
end
; frame 2, interleave 0: from frame 0, row 0 column 1 = $20
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 0                     ; column 0
b 1 $81 $20             ; column 1
end
end
; frame 3, interleave 0: from frame 1, row 1 column 0 = $30
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 2 1 $81 $30           ; column 0
b 0                     ; column 1
end
end
; frame 4, interleave 0, XOR: from frame 2, row 1 column 1 XOR $40
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 0 0                   ; interleave, pad
l 2                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 0                     ; column 0
b 2 1 $81 $40           ; column 1
end
end
; frame 5, interleave 1: from frame 4, row 0 column 0 = $50
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 1 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 1 $81 $50             ; column 0
b 0                     ; column 1
end
end
; frame 6, interleave 2: from frame 4, row 1 column 0 = $66
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 2 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 2 1 $81 $66           ; column 0
b 0                     ; column 1
end
end
; frame 7, interleave 3: from frame 4, row 0 column 1 = $77
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 3 0                   ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 0                     ; column 0
b 1 $81 $77             ; column 1
end
end
; frame 8, interleave 255: from frame 0, row 0 column 0 = $88
form ILBM
chunk ANHD
b 5 0                   ; operation 5, mask
w 16 2 0 0              ; width, height, position
l 0 1                   ; abstime, reltime
b 255 0                 ; interleave, pad
l 0                     ; bits
fill 16 0
end
chunk DLTA
l 64
fill 60 0               ; planes 1-15: none
b 1 $81 $88             ; column 0
b 0                     ; column 1
end
end
end

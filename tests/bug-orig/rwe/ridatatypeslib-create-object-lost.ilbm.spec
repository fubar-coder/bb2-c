; ridatatypeslib-create-object-lost: an ILBM of 320 x 200 pixels, four
; planes, sixteen colours, uncompressed (32000 bytes of BODY)
form ILBM
chunk BMHD
w 320 200               ; width, height
w 0 0                   ; position
b 4 0 0 0               ; planes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 200               ; page size
end
chunk CMAP
fill 48 $80             ; colours 0-15
end
chunk BODY
fill 32000 $55
end
end

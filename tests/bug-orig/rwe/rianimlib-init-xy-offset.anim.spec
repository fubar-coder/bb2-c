; rianimlib-init-xy-offset: ANIM5, 16 x 1, 1 plane, one frame
form ANIM
form ILBM
chunk BMHD
w 16 1 0 0              ; width, height, x, y
b 1 0 0 0              ; nPlanes, masking, compression, pad
w 0                     ; transparent colour
b 10 11                 ; aspect
w 320 256               ; page
end
chunk CMAP
b 0 0 0 255 255 255
end
chunk BODY              ; 1 row of 2 bytes, uncompressed
b $AA $55
end
end
end

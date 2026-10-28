; t103_ilbmerr - ilbmifflib DecodeILBM: memory without FORM/BMHD
m.l=AllocMem(64,65536)
Poke.l m,$464F524D : Poke.l m+4,12 : Poke.l m+8,$494C424D
Poke.l m+12,$43414D47 : Poke.l m+16,0
BitMap 0,32,2,1
NPrint "before"
DecodeILBM 0,m
NPrint "after"
End

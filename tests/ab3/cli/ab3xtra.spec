; AmiBlitz3's binary .xtra as its write_xtrafile writes it
; (AmiBlitz/AmiBlitz3@58a11658, AmiBlitz3.ab3 25089; docs/formats.md 1.3):
; the companion of incother_ab3tok.bb2, which makes it a tokenised source
; of AB3 mode (xtra_is_ab3_xtra)
b 1 $80                                 ; vers 1, icon
w $80 0 $80 0 $80 0 0 0 0 0 0 0 0 0 0   ; 15 switch words
l 16384 32768 4096 4096 8192 10240      ; 6 buffer sizes at 32
fill 1044 0                             ; residents at 56, executable at 600/792, CLI at 984
l 8192 0 1                              ; stack, version, builds at 1100
l 100                                   ; no {lib, max} pairs; maxerrorcount

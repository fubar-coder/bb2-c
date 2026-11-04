; AmiBlitz3's binary .xtra as its write_xtrafile writes it, with its
; default buffer sizes (Macro Buffer 40000): the companion of ab3xtra_tok.bb2.
; Read in BB2's 14-word layout the sizes are 2 bytes late (maxpc 0,
; 16384 -> $40000000); ab3cli does not use the file
b 1 $80                                 ; vers 1, icon
w $80 0 $80 0 $80 0 0 0 0 0 0 0 0 0 0   ; 15 switch words
l 16384 32768 4096 4096 40000 10240     ; 6 buffer sizes at 32, Macro Buffer at 48
fill 1044 0                             ; residents at 56, executable at 600/792, CLI at 984
l 8192 0 1                              ; stack, version, builds at 1100
l 100                                   ; no {lib, max} pairs; maxerrorcount

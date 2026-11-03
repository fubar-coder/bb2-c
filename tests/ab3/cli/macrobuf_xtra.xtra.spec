; a BB2 .xtra as blitz2's savextra writes it (blitz2.s:15412, version 1)
; with AmiBlitz3's default Macro Buffer of 40000: the companion of
; macrobuf_xtra.bb2 (#336: blitz2 frees macrobuff with this size)
b 1 0                                   ; vers 1, no icon
w $80 0 0 $80 0 0 0 $80 $80 0 0 $80 $80 0 ; 14 switch words (formats.md 1.1): runtime errors, macros, auto run, interrupts
l 16384 32768 4096 4096 40000 10240     ; 6 buffer sizes at 30, Macro Buffer at 46
fill 1056 0                             ; residents, executable name, CLI argument

; compiler-even-pad-byte-code (#969) - pokewd, which writes every word of
; code, rounds pc up to even before writing and never writes the byte it
; skips. An odd Dc.b (or IncBin) followed directly by a statement (no
; Even) leaves that pad byte as the compiler's object buffer held it:
; with runtime error checking on, the bytes of the debug record that
; debugstuff wrote at the start of the Dc.b statement and undodebug
; withdrew for the directive. With 1 data byte the pad byte is the low
; byte of the record's trap #1 ($41 here, layout-independent).
; Correct: the pad byte is 0, the program prints zero.
; @pass runtime_errors=on
Goto skip
a: Dc.b "J"
NPrint "never"
skip:
If Peek.b(?a+1)=0 Then NPrint "zero" Else NPrint "garbage ",Hex$(Peek.b(?a+1)&255)
End

; blitzoslib-boxprint-bank-lf (#87) - BoxPrint Box#,Bank#,Line finds the
; line in the bank and replaces the LF at its end by 0 in the bank itself
; (the line is printed with its length anyway).
; Here a bank with "AB",LF,"CD",LF and BoxPrint of line 0 into a view
; box.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: the bank is unchanged, the byte at +2 is still 10.
; @fixed #87
; @configs clib
BitMap 0,320,200,2
InitPage 0,0,1
AddBox 0,0,0,0,160,40,0
InitBank 0,16,65536
Poke.l Bank(0),$41420A43 : Poke.l Bank(0)+4,$440A0000
BLITZ
BitMapOutput 0
BoxPrint 0,0,0
AMIGA
DefaultOutput
NPrint "byte 2 ",Peek.b(Bank(0)+2)
End
LoadShape 0,"x"

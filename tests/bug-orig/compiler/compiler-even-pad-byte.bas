; compiler-even-pad-byte (#969) - Even (also Even4, Even8, the word
; alignment of Dc.w/.l and Dcb.w/.l) only moves pc to the next even
; address and never writes the byte it skips; Ds moves pc over the
; reserved storage the same way. The executable gets whatever the
; compiler's object buffer held there: with runtime error checking on
; the bytes of the debug record (trap #1, line item, column) that
; debugstuff wrote at the start of each statement and undodebug
; withdrew for the label and the assembler directives. With 7 data bytes
; the pad byte is the low byte of the record's column word ($0B here).
; Correct: the pad byte is 0, the program prints zero.
; @pass runtime_errors=on
Goto skip
a: Dc.b "JKLMNO",0
Even
skip:
If Peek.b(?a+7)=0 Then NPrint "zero" Else NPrint "garbage ",Hex$(Peek.b(?a+7)&255)
End

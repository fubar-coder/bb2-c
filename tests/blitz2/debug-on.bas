; @runerrs on
; #__debug with runtime error checking: -1, in expressions, CNIF, constant
; definitions and assembler operands
NPrint #__debug
CNIF #__debug
NPrint "debug build"
CEND
CNIF #__debug=0
NPrint "release build"
CEND
#d=#__debug
NPrint #d
MOVEQ #__debug,d0
PutReg d0,a.l
NPrint a
End

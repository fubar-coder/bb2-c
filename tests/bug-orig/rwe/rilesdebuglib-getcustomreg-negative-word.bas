; rilesdebuglib-getcustomreg-negative-word (#714) - GetCustomReg checks
; the register with CMPI.w #$1fc (signed word) and TST.l (the whole
; long): a word of $8000 or more with a positive long passes, and
; ASL.w #2 / ADD.w index up to 32 KB in front of the name table: "a name"
; is copied from there (code or data, up to a space or 9 characters).
; The long $0000FFF0 reads 64 bytes in front of the table.
; Correct: the register $FFF0 does not exist, the name is empty.
; @rwe rilesdebuglib
DEFTYPE .l
x=$FFF0
a$=GetCustomReg(x)
If a$="" Then NPrint "no name" Else NPrint "a name of ",Len(a$)," characters"
End

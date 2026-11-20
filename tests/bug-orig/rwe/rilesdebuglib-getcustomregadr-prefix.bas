; rilesdebuglib-getcustomregadr-prefix (#713) - GetCustomRegAdr compares
; the name with the table's until the table's space: a longer name that
; starts with a register's name matches it ("DMACONXYZ" gives $096), and
; a register whose name has 8 characters (no space) is never found, the
; compare runs into the next name ("DENISEID", "SPR7DATB").
; Correct: "DMACONXYZ" is no register (-1), DENISEID is $07C (124),
; SPR7DATB $17E (382), DMACON $096 (150).
; @fixed #713
; @rwe rilesdebuglib
DEFTYPE .l
n$="DMACONXYZ" : NPrint GetCustomRegAdr(n$)
n$="DENISEID" : NPrint GetCustomRegAdr(n$)
n$="SPR7DATB" : NPrint GetCustomRegAdr(n$)
n$="DMACON" : NPrint GetCustomRegAdr(n$)
End

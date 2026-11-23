; rilesdebuglib-getcustomregadr-prefix (#713) - GetCustomRegAdr compares
; the name with the table's until the table's space: a longer name that
; starts with a register's name matches it ("DMACONXYZ" gives $096), and
; a register whose name has 8 characters (no space) is never found, the
; compare runs into the next name ("DENISEID", "SPR7DATB").
; Correct: "DMACONXYZ" is no register (-1), DENISEID is $07C (124),
; SPR7DATB $17E (382), DMACON $096 (150).
DEFTYPE .l

!TestCase{names}
  n$="DMACONXYZ" : r1=GetCustomRegAdr(n$)
  n$="DENISEID" : r2=GetCustomRegAdr(n$)
  n$="SPR7DATB" : r3=GetCustomRegAdr(n$)
  n$="DMACON" : r4=GetCustomRegAdr(n$)
  !CheckEq{r1,-1,"longer_name"}
  !CheckEq{r2,124,"deniseid"}
  !CheckEq{r3,382,"spr7datb"}
  !CheckEq{r4,150,"dmacon"}
!EndTestCase
